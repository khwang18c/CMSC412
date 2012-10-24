#include <geekos/syscall.h>
#include <geekos/errno.h>
#include <geekos/kthread.h>
#include <geekos/int.h>
#include <geekos/elf.h>
#include <geekos/malloc.h>
#include <geekos/screen.h>
#include <geekos/keyboard.h>
#include <geekos/string.h>
#include <geekos/user.h>
#include <geekos/timer.h>
#include <geekos/vfs.h>
#include <geekos/signal.h>
#include <geekos/sem.h>
#include <geekos/projects.h>


/*
 * Global Semaphore list
 */
static struct Semaphore *sem_list[MAX_SEM_SIZE] = { 0 };

/*
 * Create or find a semaphore.
 * Params:
 *   state->ebx - user address of name of semaphore
 *   state->ecx - length of semaphore name
 *   state->edx - initial semaphore count
 * Returns: the global semaphore id
 */
int Sys_Open_Semaphore(struct Interrupt_State *state) {
    int len = state->ecx;
    int ival = state->edx;

    // Name of semaphore is too long
    if (len > MAX_NAME_SIZE)
        return ENAMETOOLONG;

    char *name = Malloc(len + 1); // Holds name of semaphore
    Copy_From_User(name, state->ebx, len + 1); 
    struct Kernel_Thread *current = g_currentThread;

    // Searches through global list of semaphores
    int sem_ind = 0;
    while (sem_list[sem_ind] != 0) {

        // If the name exists
        if (strncmp(sem_list[sem_ind]->name, name, len) == 0) {
            current->userContext->sem_flags[sem_ind] = 1;
            sem_list[sem_ind]->num_users++;
            Free(name);
            return sem_list[sem_ind]->SID; 
        }
        
        sem_ind++; // increment counter
        
        // No more SIDs available and no semaphore with name exists
        if (sem_ind == MAX_SEM_SIZE){
            Free(name);
            return ENOSPACE;
        }
    }

    // Create new semaphore and add to global semaphore list
    struct Semaphore *new_sem = Malloc(sizeof(struct Semaphore));
    Clear_Thread_Queue(&new_sem->blockQueue);
    new_sem->name = name;
    new_sem->SID = sem_ind;
    new_sem->value = ival;
    new_sem->num_users = 1;
    current->userContext->sem_flags[sem_ind] = 1;
    sem_list[sem_ind] = new_sem;
    return sem_ind;
}

/*
 * Acquire a semaphore.
 * Assume that the process has permission to access the semaphore,
 * the call will block until the semaphore count is >= 0.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
int Sys_P(struct Interrupt_State *state) {
    int SID = state->ebx;
    struct Kernel_Thread *current = g_currentThread;

    // Checks if valid SID or if current thread has sem with SID open
    if (SID < 0 || SID >= MAX_SEM_SIZE || sem_list[SID] == 0 || 
        current->userContext->sem_flags[SID] == 0)
        return EINVALID;

    bool iflag = Begin_Int_Atomic();  

    // If semaphore is available
    if (sem_list[SID]->value > 0) {
        sem_list[SID]->value--;
        End_Int_Atomic(iflag);
        return 0;
    }

    // Wait on semaphore's wait queue
    Wait(&sem_list[SID]->blockQueue);

    iflag = Begin_Int_Atomic();

    sem_list[SID]->value--;

    End_Int_Atomic(iflag);

    return 0;
}

/*
 * Release a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
int Sys_V(struct Interrupt_State *state) {
    int SID = state->ebx;
    struct Kernel_Thread *current = g_currentThread;

    // Checks if valid SID or if current thread has sem with SID open
    if (SID < 0 || SID >= MAX_SEM_SIZE || sem_list[SID] == 0 || 
        current->userContext->sem_flags[SID] == 0)
        return EINVALID;

    bool iflag = Begin_Int_Atomic();

    sem_list[SID]->value++;
    if (!Is_Thread_Queue_Empty(&sem_list[SID]->blockQueue))
        Wake_Up_One(&sem_list[SID]->blockQueue); 

    End_Int_Atomic(iflag);

    return 0;
}

/*
 * Destroy our reference to a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
int Sys_Close_Semaphore(struct Interrupt_State *state) {
    return Close_Sem_Helper(state->ebx, g_currentThread);
}

/*
 * A helper function that closes the semaphore with SID for thread current 
 */
int Close_Sem_Helper(int SID, struct Kernel_Thread *current) {
     // Checks if valid SID or if current thread has sem with SID open
    if (SID < 0 || SID >= MAX_SEM_SIZE || sem_list[SID] == 0 || 
        current->userContext->sem_flags[SID] == 0)
        return EINVALID;

    // Closes semaphore to current thread
    current->userContext->sem_flags[SID] = 0;
    sem_list[SID]->num_users--;
    
    // If no users have the semaphore open, remove semaphore
    if (sem_list[SID]->num_users == 0) {
        Free(sem_list[SID]->name);
        Free(sem_list[SID]);
        sem_list[SID] = 0;
    }
    
    return 0;
}
