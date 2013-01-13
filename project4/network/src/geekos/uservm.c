/*
 * Paging-based user mode implementation
 * Copyright (c) 2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.51 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/paging.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/argblock.h>
#include <geekos/kthread.h>
#include <geekos/range.h>
#include <geekos/vfs.h>
#include <geekos/user.h>


int userDebug = 0;
#define Debug(args...) if (userDebug) Print("uservm: " args)

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */

void *User_To_Kernel(struct User_Context *userContext, ulong_t userPtr) {
    return (void *)(USER_VM_START + userPtr);
}

/*
 * Create a new user context
 */
static struct User_Context *Create_User_Context() {
    struct User_Context *context;

    int index;

    /* Allocate memory for the user context and clear it */
    Disable_Interrupts();
    context = Malloc(sizeof(struct User_Context));
    Enable_Interrupts();
    if (context == 0) 
        goto fail;
    memset(context, '\0', sizeof(struct User_Context));

    /* Allocate an LDT descriptor for the user context */
    context->ldtDescriptor = Allocate_Segment_Descriptor();
    if (context->ldtDescriptor == 0)
        goto fail;
    if (userDebug)
        Print("Allocated descriptor %d for LDT\n",
              Get_Descriptor_Index(context->ldtDescriptor));
    Init_LDT_Descriptor(context->ldtDescriptor, context->ldt,
                        NUM_USER_LDT_ENTRIES);
    index = Get_Descriptor_Index(context->ldtDescriptor);
    context->ldtSelector = Selector(KERNEL_PRIVILEGE, true, index);

    /* Initialize code and data segments within the LDT */
    Init_Code_Segment_Descriptor(&context->ldt[0], USER_VM_START,
                                 ((USER_VM_END - USER_VM_START)+1) / PAGE_SIZE, USER_PRIVILEGE);
    Init_Data_Segment_Descriptor(&context->ldt[1], USER_VM_START,
                                 ((USER_VM_END - USER_VM_START)+1) / PAGE_SIZE, USER_PRIVILEGE);
    context->csSelector = Selector(USER_PRIVILEGE, false, 0);
    context->dsSelector = Selector(USER_PRIVILEGE, false, 1);

    /* Nobody is using this user context yet */
    context->refCount = 0;

    /* Success! */
    return context;

  fail:
    /* Release memory on failure */
    Disable_Interrupts();
    if (context != 0)
        Free(context);
    Enable_Interrupts();

    return 0;
}

bool Validate_User_Memory(struct User_Context * userContext,
                          ulong_t userAddr, ulong_t bufSize) {
    ulong_t avail;

    if (userAddr >= (USER_VM_END - USER_VM_START))
        return false;

    avail = (USER_VM_END - USER_VM_START) - userAddr;
    if (bufSize > avail)
        return false;

    return true;
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */
/*
 * Destroy a User_Context object, including all memory
 * and other resources allocated within it.
 */
void Destroy_User_Context(struct User_Context *context) {
    /*
     * Hints:
     * - Free all pages, page tables, and page directory for
     *   the process (interrupts must be disabled while you do this,
     *   otherwise those pages could be stolen by other processes)
     * - Free semaphores, files, and other resources used
     *   by the process
     */
    int index = PAGE_DIRECTORY_INDEX(USER_VM_START);
    int index2 = 0;
    pde_t *pageDir;
    pte_t *pageTbl;
    void *page;
    KASSERT(context->refCount == 0);

    /* Free the context's LDT descriptor */
    Free_Segment_Descriptor(context->ldtDescriptor);

    /* Free memory allocated for all pages in page directory */
    Disable_Interrupts();
    pageDir = context->pageDir;
    while (index < NUM_PAGE_DIR_ENTRIES) {
        if (pageDir[index].present == 1) {
            pageTbl = (pte_t *)PAGE_ORIG(pageDir[index].pageTableBaseAddr); 
            index2 = 0;
            while(index2 < NUM_PAGE_TABLE_ENTRIES) {
                if (pageTbl[index2].present == 1) 
                    Free_Page(PAGE_ORIG(pageTbl[index2].pageBaseAddr));
                index2++;
            }
            Free_Page(pageTbl);
        }
        index++;
    } 

    /* Free page directory */
    Free_Page(context->pageDir);

    /* Free memory allocated for the user context */
    Free(context);

    Enable_Interrupts();
}

/*
 * Load a user executable into memory by creating a User_Context
 * data structure.
 * Params:
 * exeFileData - a buffer containing the executable to load
 * exeFileLength - number of bytes in exeFileData
 * exeFormat - parsed ELF segment information describing how to
 *   load the executable's text and data segments, and the
 *   code entry point address
 * command - string containing the complete command to be executed:
 *   this should be used to create the argument block for the
 *   process
 * pUserContext - reference to the pointer where the User_Context
 *   should be stored
 *
 * Returns:
 *   0 if successful, or an error code (< 0) if unsuccessful
 */
int Load_User_Program(char *exeFileData, ulong_t exeFileLength,
                      struct Exe_Format *exeFormat, const char *command,
                      struct User_Context **pUserContext) {
    /*
     * Hints:
     * - This will be similar to the same function in userseg.c
     * - Determine space requirements for code, data, argument block,
     *   and stack
     * - Allocate pages for above, map them into user address
     *   space (allocating page directory and page tables as needed)
     * - Fill in initial stack pointer, argument block address,
     *   and code entry point fields in User_Context
     */
    int i;
    unsigned numArgs;
    ulong_t argBlockSize;
    ulong_t argBlockAddr;
    struct User_Context *userContext = 0;
    pde_t *userPD;
    void *page;
    ulong_t pageOffset, copyLeft, vaddr, copySize, copied;
    struct Exe_Segment *segment;
    int d;

    /* Create User_Context */
    userContext = Create_User_Context();
    if (userContext == 0)
        return -1;

    /* Allocate user page directory and clear it */
    userPD = Alloc_Page();
    if (userPD == 0) 
        Exit(-1);
    memset(userPD, '\0', PAGE_SIZE);

    /* Set page directory */
    userContext->pageDir = userPD;

    /* Copy kernel page directory to user page directory */
    for (d = 0; d < NUM_PAGE_DIR_ENTRIES/2; d++) {
        if(g_pageDir[d].present == 1)
            userContext->pageDir[d] = g_pageDir[d]; 
    }
    
    /* Load segment data into memory */
    for (i = 0; i < exeFormat->numSegments; i++) {
        segment = &exeFormat->segmentList[i];        
        vaddr = LOGICAL_TO_LINEAR(segment->startAddress); // Virtual address
        pageOffset = vaddr % PAGE_SIZE; // Offset from physical page base addr
        copyLeft = segment->lengthInFile; // Bytes needed to be copied
        copied = 0; // How many bytes copied so far

        /* Skip if zero segment */
        if (PAGE_ADDR(segment->startAddress) == 0)
            continue;
        else {
            while (copyLeft > 0) {
                /* Claim a page */
                page = Claim_Page(vaddr, userPD);
                /* Determine how much bytes should be copied into page */
                if (copyLeft > (PAGE_SIZE - pageOffset))
                    copySize = PAGE_SIZE;
                else
                    copySize = copyLeft;
                /* Copy segment data into physical page */
                memcpy(page + pageOffset, 
                       exeFileData + segment->offsetInFile + copied,
                       copySize);
                Unclaim_Page(vaddr, userPD);
                pageOffset = 0; // No more page offset 
                copied = copySize; // Bytes copied so far
                copyLeft = copyLeft - copySize; // Bytes left to copy
                vaddr += PAGE_SIZE; // Ensures next page will be grabbed
            }
        }
    }

    /* Determine size required for argument block */
    Get_Argument_Block_Size(command, &numArgs, &argBlockSize);

    /* Allocate page for argument block */
    page = Claim_Page(USER_ARGS, userPD);
    Unclaim_Page(USER_ARGS, userPD);

    /* Format argument block */
    Format_Argument_Block(page, numArgs,
                          USER_ARGS - USER_VM_START, command);

    /* Allocate page for stack */
    page = Claim_Page(USER_STACK, userPD);
    Unclaim_Page(USER_STACK, userPD);

    /* Fill in code entry point */
    userContext->entryAddr = exeFormat->entryAddr;

    /*
     * Fill in addresses of argument block and stack
     * (They happen to be the same)
     */
    userContext->argBlockAddr = USER_ARGS - USER_VM_START;
    userContext->stackPointerAddr = USER_ARGS - USER_VM_START;

    /* Initial stack limit */
    userContext->stackLimit = USER_STACK;

    *pUserContext = userContext;

    return 0;
}

/*
 * Copy data from user buffer into kernel buffer.
 * Returns true if successful, false otherwise.
 */
bool Copy_From_User(void *destInKernel, ulong_t srcInUser, ulong_t bufSize) {
    /*
     * Hints:
     * - Make sure that user page is part of a valid region
     *   of memory
     * - Remember that you need to add 0x80000000 to user addresses
     *   to convert them to kernel addresses, because of how the
     *   user code and data segments are defined
     * - User pages may need to be paged in from disk before being accessed.
     * - Before you touch (read or write) any data in a user
     *   page, **disable the PAGE_PAGEABLE bit**.
     *
     * Be very careful with race conditions in reading a page from disk.
     * Kernel code must always assume that if the struct Page for
     * a page of memory has the PAGE_PAGEABLE bit set,
     * IT CAN BE STOLEN AT ANY TIME.  The only exception is if
     * interrupts are disabled; because no other process can run,
     * the page is guaranteed not to be stolen.
     */
    struct User_Context *current = g_currentThread->userContext;

    if (!Validate_User_Memory(current, srcInUser, bufSize))
        return false;
    Claim_Page(LOGICAL_TO_LINEAR(srcInUser), current->pageDir);
    memcpy(destInKernel, User_To_Kernel(current, srcInUser), bufSize);
    Unclaim_Page(LOGICAL_TO_LINEAR(srcInUser), current->pageDir);

    return true;
}

/*
 * Copy data from kernel buffer into user buffer.
 * Returns true if successful, false otherwise.
 */
bool Copy_To_User(ulong_t destInUser, void *srcInKernel, ulong_t bufSize) {
    /*
     * Hints:
     * - Same as for Copy_From_User()
     * - Also, make sure the memory is mapped into the user
     *   address space with write permission enabled
     */
    struct User_Context *current = g_currentThread->userContext;

    if (!Validate_User_Memory(current, destInUser, bufSize))
        return false;
    Claim_Page(LOGICAL_TO_LINEAR(destInUser), current->pageDir);
    memcpy(User_To_Kernel(current, destInUser), srcInKernel, bufSize);
    Unclaim_Page(LOGICAL_TO_LINEAR(destInUser), current->pageDir);

    return true;
}

/*
 * Switch to user address space.
 */
void Switch_To_Address_Space(struct User_Context *userContext) {
    /*
     * - If you are still using an LDT to define your user code and data
     *   segments, switch to the process's LDT
     * - 
     */
    ushort_t ldtSelector;

    /* Switch to the LDT of the new user context */
    ldtSelector = userContext->ldtSelector;
    __asm__ __volatile__("lldt %0"::"a"(ldtSelector)
        );
    
    Set_PDBR(userContext->pageDir);
}
