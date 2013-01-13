#include <geekos/list.h>
#include <geekos/kthread.h>

#ifndef _INCLUDED_SEM_H
#define _INCLUDED_SEM_H
#ifdef GEEKOS

#define MAX_NAME_SIZE 25 // Max name size
#define MAX_SEM_SIZE 20 // Max number of semaphores

// Semaphore structure
struct Semaphore {
    char *name;
    int SID;
    int value;
    int num_users;
    struct Thread_Queue blockQueue;
};
    
int Sys_Open_Semaphore(struct Interrupt_State *state);
int Sys_P(struct Interrupt_State *state);
int Sys_V(struct Interrupt_State *state);
int Sys_Close_Semaphore(struct Interrupt_State *state);
int Close_Sem_Helper(int SID, struct Kernel_Thread *current);
#endif
#endif
