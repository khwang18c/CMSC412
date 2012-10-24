/*************************************************************************/
/*
 * GeekOS master source distribution and/or project solution
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 *
 * This file is not distributed under the standard GeekOS license.
 * Publication or redistribution of this file without permission of
 * the author(s) is prohibited.
 */
/*************************************************************************/
/*
 * Signals
 * $Rev $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/kassert.h>
#include <geekos/defs.h>
#include <geekos/screen.h>
#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/symbol.h>
#include <geekos/string.h>
#include <geekos/kthread.h>
#include <geekos/malloc.h>
#include <geekos/user.h>
#include <geekos/signal.h>
#include <geekos/projects.h>
#include <geekos/errno.h>

/* Called when signal handling is complete. */
void Complete_Handler(struct Kernel_Thread *kthread,
                      struct Interrupt_State *state) {
    KASSERT(kthread);
    KASSERT(state);

    struct User_Interrupt_State *user = (struct User_Interrupt_State *) state;

    kthread->userContext->handling_signal = 0;
    
    user->espUser += sizeof(int); // Pop off signal num on user stack
    if(!Copy_From_User(state, user->espUser, sizeof(struct Interrupt_State)))
        return EUNSPECIFIED;
    
    user->espUser += sizeof(struct Interrupt_State); // Change user esp
}

/* Check if a signal is pending */
int Check_Pending_Signal(struct Kernel_Thread *kthread,
                         struct Interrupt_State *state) {
    if (kthread->userContext->signal_pending && (state->cs != KERNEL_CS) &&
        !kthread->userContext->handling_signal)
        return 1;

    return 0;
}

/* Sets the signal handler */
int Set_Handler(signal_handler sighandler, int signum) {
    g_currentThread->userContext->sighandlers[signum - 1] = sighandler;
    return 0;
}

/* Send signum to kthread */
void Send_Signal(struct Kernel_Thread *kthread, int signum) {
    struct Signal_Info *currSig = Get_Front_Of_Signal_Queue(
                                        &kthread->userContext->sigQueue);
    struct Signal_Info *newSig;  
    
    /* Checks for duplicates in the signal queue */
    while (currSig != 0) {
        if (currSig->signalnum == signum){
            return;
        }
        
        currSig = Get_Next_In_Signal_Queue(currSig);
    }

    /* Allocate memory for the Signal_Info struct */
    newSig = Malloc(sizeof(struct Signal_Info));
   
    /* Set up the new signal struct to be added to queue */
    newSig->signalnum = signum;

    /* Set the thread's signal pending flag */
    kthread->userContext->signal_pending = 1;
    
    /* If signal is SIGKILL then add to front of queue */
    if (signum == SIGKILL) {
        Add_To_Front_Of_Signal_Queue(&kthread->userContext->sigQueue, newSig); 
    }
    /* Else just add to back of queue */
    else {
        Add_To_Back_Of_Signal_Queue(&kthread->userContext->sigQueue, newSig);
    }
}

#if 0
    Print("esp=%x:\n", (unsigned int)esp);
    Print("  gs=%x\n", (unsigned int)esp->gs);
    Print("  fs=%x\n", (unsigned int)esp->fs);
    Print("  es=%x\n", (unsigned int)esp->es);
    Print("  ds=%x\n", (unsigned int)esp->ds);
    Print("  ebp=%x\n", (unsigned int)esp->ebp);
    Print("  edi=%x\n", (unsigned int)esp->edi);
    Print("  esi=%x\n", (unsigned int)esp->esi);
    Print("  edx=%x\n", (unsigned int)esp->edx);
    Print("  ecx=%x\n", (unsigned int)esp->ecx);
    Print("  ebx=%x\n", (unsigned int)esp->ebx);
    Print("  eax=%x\n", (unsigned int)esp->eax);
    Print("  intNum=%x\n", (unsigned int)esp->intNum);
    Print("  errorCode=%x\n", (unsigned int)esp->errorCode);
    Print("  eip=%x\n", (unsigned int)esp->eip);
    Print("  cs=%x\n", (unsigned int)esp->cs);
    Print("  eflags=%x\n", (unsigned int)esp->eflags);
    p = (void **)(((struct Interrupt_State *)esp) + 1);
    Print("esp+n=%x\n", (unsigned int)p);
    Print("esp+n[0]=%x\n", (unsigned int)p[0]);
    Print("esp+n[1]=%x\n", (unsigned int)p[1]);
}

void dump_stack(unsigned int *esp, unsigned int ofs) {
    int i;
    Print("Setup_Frame: Stack dump\n");
    for (i = 0; i < 25; i++) {
        Print("[%x]: %x\n", (unsigned int)&esp[i] - ofs, esp[i]);
    }
}
#endif

/* Sets up user and kernel stack for signal handling */
void Setup_Frame(struct Kernel_Thread *kthread, struct Interrupt_State *state) {
    KASSERT(kthread);
    KASSERT(state);

    kthread->userContext->handling_signal = 1;
    
    struct User_Interrupt_State *user = (struct User_Interrupt_State *) state;
    struct Signal_Info *currSig = Remove_From_Front_Of_Signal_Queue(
                    &kthread->userContext->sigQueue);
    int signum = currSig->signalnum;
    signal_handler sigHand = kthread->userContext->sighandlers[signum - 1];
    
    /* Free up the space that was malloced for the signal info struct */
    Free(currSig);
    
    /* Checks if there are any more signals in the signal_queue */
    if (Is_Signal_Queue_Empty(&kthread->userContext->sigQueue))
        kthread->userContext->signal_pending = 0;

    /* Return if signal_handler is SIG_IGN */
    if (sigHand == SIG_IGN) {
        kthread->userContext->handling_signal = 0;
        return;
    } 
    /* Handle if signal_handler is SIG_DFL */ 
    else if (sigHand == SIG_DFL) {
        kthread->userContext->handling_signal = 0;
        if (signum == SIGCHLD) { 
            return;
        }
        
        Print("Terminated %d\n", g_currentThread->pid);
        Exit(-1);
    } 
    /* Else set up the user and kernel stack */ 
    else {
        /* Push interrupt_state onto user stack */
        user->espUser -= sizeof(struct Interrupt_State); 
        if(!Copy_To_User(user->espUser, state, sizeof(struct Interrupt_State)))
            return EUNSPECIFIED;
         
        /* Push the signal number onto user stack */
        user->espUser -= sizeof(int); 
        if(!Copy_To_User(user->espUser, &signum, sizeof(int)))
            return EUNSPECIFIED;
           
        /* Push the signal trampoline onto user stack */
        user->espUser -= sizeof(signal_handler);
        if(!Copy_To_User(user->espUser, &kthread->userContext->returnSig, sizeof(signal_handler)))
            return EUNSPECIFIED;
        
        /* Change the kernel stack */
        state->eip = sigHand;
    }
}
