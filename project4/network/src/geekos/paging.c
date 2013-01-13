/*
 * Paging (virtual memory) support
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.56 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/string.h>
#include <geekos/int.h>
#include <geekos/idt.h>
#include <geekos/kthread.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/mem.h>
#include <geekos/malloc.h>
#include <geekos/gdt.h>
#include <geekos/segment.h>
#include <geekos/user.h>
#include <geekos/vfs.h>
#include <geekos/crc32.h>
#include <geekos/paging.h>

/* ----------------------------------------------------------------------
 * Public data
 * ---------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
 * Private functions/data
 * ---------------------------------------------------------------------- */

#define SECTORS_PER_PAGE (PAGE_SIZE / SECTOR_SIZE)
#define KB_IN_PAGE PAGE_SIZE/1024

/*
 * flag to indicate if debugging paging code
 */
int debugFaults = 1;
#define Debug(args...) if (debugFaults) Print(args)

/* Global kernel page directory */
pde_t *g_pageDir;

/* Global page file */
pagefile *g_pagefile;

/*
 * Print diagnostic information for a page fault.
 */
static void Print_Fault_Info(uint_t address, faultcode_t faultCode) {
    extern uint_t g_freePageCount;

    Print("Pid %d, Page Fault received, at address %x (%d pages free)\n",
          g_currentThread->pid, address, g_freePageCount);
    if (faultCode.protectionViolation)
        Print("   Protection Violation, ");
    else
        Print("   Non-present page, ");
    if (faultCode.writeFault)
        Print("Write Fault, ");
    else
        Print("Read Fault, ");
    if (faultCode.userModeFault)
        Print("in User Mode\n");
    else
        Print("in Supervisor Mode\n");
}

/*
 * Handler for page faults.
 * You should call the Install_Interrupt_Handler() function to
 * register this function as the handler for interrupt 14.
 */
/*static*/ void Page_Fault_Handler(struct Interrupt_State *state) {
    ulong_t address;
    faultcode_t faultCode;
    struct User_Context *curr = g_currentThread->userContext;
    ulong_t paddr;
    int pdIndex;
    int ptIndex;
    int pagefileIndex;
    pte_t *pageTbl;
    pde_t *pageDir = curr->pageDir;

    KASSERT(!Interrupts_Enabled());

    /* Get the address that caused the page fault */
    address = Get_Page_Fault_Address();

    /* Get info about the page entry */
    pdIndex = PAGE_DIRECTORY_INDEX(address);
    ptIndex = PAGE_TABLE_INDEX(address);
    pageTbl = PAGE_ORIG(pageDir[pdIndex].pageTableBaseAddr);

    /* Check if address is within one page from current stack limit 
       and allocate a new page if it is */
    if (address < curr->stackLimit && address > curr->stackLimit - PAGE_SIZE) {
        Print("Increase stack\n");
        paddr = Claim_Page(address, pageDir);
        Unclaim_Page(address, pageDir);
        curr->stackLimit = curr->stackLimit - PAGE_SIZE;
        return;
    } else if (pageTbl[ptIndex].kernelInfo == KINFO_PAGE_ON_DISK) {
        Print("Read from pagefile to vaddr: %08X\n", address);
        pagefileIndex = pageTbl[ptIndex].pageBaseAddr;
        paddr = Claim_Page(address, curr->pageDir);  
        Enable_Interrupts(); 
        Read_From_Paging_File(paddr, address, pagefileIndex);
        Disable_Interrupts();
        pageTbl[ptIndex].kernelInfo = 0;
        Unclaim_Page(address, curr->pageDir);
        Free_Space_On_Paging_File(pagefileIndex);
        return;
    }

    Debug("Page fault @%lx\n", address);

    /* Get the fault code */
    faultCode = *((faultcode_t *) & (state->errorCode));

    /* rest of your handling code here */
    Print("Unexpected Page Fault received\n");
    Print_Fault_Info(address, faultCode);
    Dump_Interrupt_State(state);
    /* user faults just kill the process */
    if (!faultCode.userModeFault)
        KASSERT(0);

    /* For now, just kill the thread/process. */
    Exit(-1);
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

/* 
 * Allocates page table and page if necessary. 
 * Returns the physical address of the page
 */
void *Claim_Page(ulong_t vaddr, pde_t *pageDir) {
    pte_t *pageTbl;
    void *paddr = 0;
    int pdIndex = PAGE_DIRECTORY_INDEX(vaddr);
    int ptIndex = PAGE_TABLE_INDEX(vaddr);
    struct Page *page = 0;

    bool flag = Begin_Int_Atomic();
    /* If no page dir entry exists */
    if (pageDir[pdIndex].present == 0) {
        pageTbl = Alloc_Page(); 
        if (pageTbl == 0)
            Exit(-1);
        memset(pageTbl, '\0', PAGE_SIZE);
        pageDir[pdIndex].present = 1;
        pageDir[pdIndex].flags = VM_USER | VM_WRITE | VM_READ;
        pageDir[pdIndex].pageTableBaseAddr = PAGE_ALIGNED_ADDR(pageTbl);
    } 

    /* Get the original page tbl address back */
    pageTbl = PAGE_ORIG(pageDir[pdIndex].pageTableBaseAddr);

    /* If no page table entry exists */
    if (pageTbl[ptIndex].present == 0) {
        paddr = Alloc_Pageable_Page(&pageTbl[ptIndex], PAGE_ADDR(vaddr));
        if (paddr == 0)
            Exit(-1);
        memset(paddr, '\0', PAGE_SIZE);
        pageTbl[ptIndex].present = 1;
        pageTbl[ptIndex].flags = VM_USER | VM_WRITE | VM_READ;
        pageTbl[ptIndex].pageBaseAddr = PAGE_ALIGNED_ADDR(paddr);
    }

    /* Get physical page base address */
    paddr = PAGE_ORIG(pageTbl[ptIndex].pageBaseAddr);

    page = Get_Page(paddr);                
    page->flags &= ~(PAGE_PAGEABLE); // Make page unpageable
    End_Int_Atomic(flag);

    return paddr;
}

/* 
 * Unclaims a page
 */
void Unclaim_Page(ulong_t vaddr, pde_t *pageDir) {
    int pdIndex = PAGE_DIRECTORY_INDEX(vaddr);
    int ptIndex = PAGE_TABLE_INDEX(vaddr);
    ulong_t paddr = 0;
    struct Page *page = 0;
    pte_t *pageTbl;
    bool flag = Begin_Int_Atomic();

    pageTbl = PAGE_ORIG(pageDir[pdIndex].pageTableBaseAddr);
    paddr = PAGE_ORIG(pageTbl[ptIndex].pageBaseAddr);
    page = Get_Page(paddr);
    page->flags |= PAGE_PAGEABLE; // Make page pageable
    End_Int_Atomic(flag);
}

///*
// * Initialize virtual memory by building page tables
// * for the kernel and physical memory.
// */
void Init_VM(struct Boot_Info *bootInfo) {
    /*
     * Hints:
     * - Build kernel page directory and page tables
     * - Call Enable_Paging() with the kernel page directory
     * - Install an interrupt handler for interrupt 14,
     *   page fault
     * - Do not map a page at address 0; this will help trap
     *   null pointer references
     */
    pte_t *pageTbl;
    int ptInd, pdInd;
    int pageEntries = bootInfo->memSizeKB/4; // number of pd's needed
    if (bootInfo->memSizeKB % 4 != 0)
        pageEntries++;
    int count = 0;

    /* Allocate global page directory and clear it */
    g_pageDir = Alloc_Page();
    if (g_pageDir == 0)
        Exit(-1);
    memset(g_pageDir, '\0', PAGE_SIZE);

    /* Identity map all physical memory available */
    for (pdInd = 0; pdInd < NUM_PAGE_DIR_ENTRIES/2; pdInd++) {
        if (count < pageEntries) {
            /* Allocate new page table and clear it */
            pageTbl = Alloc_Page(); 
            if (pageTbl == 0)
                Exit(-1);
            memset(pageTbl, '\0', PAGE_SIZE);

            /* Put in page dir entry */
            g_pageDir[pdInd].present = 1;
            g_pageDir[pdInd].flags = VM_WRITE | VM_READ;
            g_pageDir[pdInd].pageTableBaseAddr = PAGE_ALIGNED_ADDR(pageTbl);
            
            /* Add in page table entries for the page dir entry just made*/
            for (ptInd = 0; ptInd < NUM_PAGE_TABLE_ENTRIES; ptInd++) {
                if (count < pageEntries) {
                    /* Put in page table entry */
                    pageTbl[ptInd].present = 1;
                    pageTbl[ptInd].flags = VM_WRITE | VM_READ;
                    pageTbl[ptInd].pageBaseAddr = PAGE_ALIGNED_ADDR(count * PAGE_SIZE);

                    count++;
                } else {
                    break;
                }
            }
        } else {
            break;
        }
    }

    ((pte_t *)PAGE_ORIG(g_pageDir[0].pageTableBaseAddr))[0].present = 0;
    
    /* Enable paging */
    Enable_Paging(g_pageDir);

    /* Add page fault interrupt handler */
    Install_Interrupt_Handler(14, Page_Fault_Handler);
}

/**
 * Initialize paging file data structures.
 * All filesystems should be mounted before this function
 * is called, to ensure that the paging file is available.
 */
void Init_Paging(void) {
    struct Paging_Device *pd = Get_Paging_Device();
    int index;
    g_pagefile = Malloc(sizeof(pagefile));
    g_pagefile->dev = pd->dev;
    g_pagefile->startSector = pd->startSector;
    g_pagefile->numSectors = pd->numSectors;
    for (index = 0; index < NUM_PAGE; index++) {
        g_pagefile->used[index] = 0;
    }
}

/**
 * Find a free bit of disk on the paging file for this page.
 * Interrupts must be disabled.
 * @return index of free page sized chunk of disk space in
 *   the paging file, or -1 if the paging file is full
 */
int Find_Space_On_Paging_File(void) {
    KASSERT(!Interrupts_Enabled());
    int index = 0;
    for (index = 0; index < NUM_PAGE; index++) {
        if (g_pagefile->used[index] == 0) { 
            g_pagefile->used[index] = 1;
            return index * SECTORS_PER_PAGE;
        }
    }
    return -1;
}

/**
 * Free a page-sized chunk of disk space in the paging file.
 * Interrupts must be disabled.
 * @param pagefileIndex index of the chunk of disk space
 */
void Free_Space_On_Paging_File(int pagefileIndex) {
    KASSERT(!Interrupts_Enabled());
    g_pagefile->used[pagefileIndex/SECTORS_PER_PAGE] = 0;
}

/**
 * Write the contents of given page to the indicated block
 * of space in the paging file.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page is mapped in user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Write_To_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex) {
    struct Page *page = Get_Page((ulong_t) paddr);
    int index = 0;
    KASSERT(!(page->flags & PAGE_PAGEABLE));    /* Page must be locked! */
    
    for (index = 0; index < SECTORS_PER_PAGE; index++) {
        Block_Write(g_pagefile->dev, 
                    g_pagefile->startSector + pagefileIndex + index,
                    paddr + (index * SECTOR_SIZE));
    }
    g_pagefile->used[pagefileIndex/SECTORS_PER_PAGE] = 1; 
}

/**
 * Read the contents of the indicated block
 * of space in the paging file into the given page.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page will be re-mapped in
 *   user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Read_From_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex) {
    struct Page *page = Get_Page((ulong_t) paddr);
    KASSERT(!(page->flags & PAGE_PAGEABLE));    /* Page must be locked! */
    int index = 0;
    for (index = 0; index < SECTORS_PER_PAGE; index++) {
        Block_Read(g_pagefile->dev, 
                   g_pagefile->startSector + pagefileIndex + index,
                   paddr + (index * SECTOR_SIZE));
    }
}
