/*
 * Kernel threads
 * Copyright (c) 2001,2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.55 $
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
#include <geekos/alarm.h>
#include <geekos/projects.h>


/* ----------------------------------------------------------------------
 * Private data
 * ---------------------------------------------------------------------- */

/*
 * List of all threads in the system.
 */
static struct All_Thread_List s_allThreadList;

/*
 * Queue of runnable threads.
 */
static struct Thread_Queue s_runQueue;

/*
 * Current thread.
 */
struct Kernel_Thread *g_currentThread;

/*
 * Boolean flag indicating that we need to choose a new runnable thread.
 * It is checked by the interrupt return code (Handle_Interrupt,
 * in lowlevel.asm) before returning from an interrupt.
 */
int g_needReschedule;

/*
 * Boolean flag indicating that preemption is disabled.
 * When set, external interrupts (such as the timer tick)
 * will not cause a new thread to be selected.
 */
volatile int g_preemptionDisabled;

/*
 * Queue of finished threads needing disposal,
 * and a wait queue used for communication between exited threads
 * and the reaper thread.
 */
static struct Thread_Queue s_graveyardQueue;
static struct Thread_Queue s_reaperWaitQueue;

/*
 * Counter for keys that access thread-local data, and an array
 * of destructors for freeing that data when the thread dies.  This is
 * based on POSIX threads' thread-specific data functionality.
 */
static unsigned int s_tlocalKeyCounter = 0;
static tlocal_destructor_t s_tlocalDestructors[MAX_TLOCAL_KEYS];

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */

/*
 * Initialize a new Kernel_Thread.
 */
static void Init_Thread(struct Kernel_Thread *kthread, void *stackPage,
                        int priority) {
    static int nextFreePid = 1;

    TODO_P(PROJECT_BACKGROUND_JOBS, "Add detached argument handling here");
    struct Kernel_Thread *owner = g_currentThread;

    memset(kthread, '\0', sizeof(*kthread));
    kthread->stackPage = stackPage;
    kthread->esp = ((ulong_t) kthread->stackPage) + PAGE_SIZE;
    kthread->numTicks = 0;
    kthread->priority = priority;
    kthread->userContext = 0;
    kthread->owner = owner;

    /*
     * The thread has an implicit self-reference.
     * If the thread is not detached, then its owner
     * also has a reference to it.
     */
    TODO_P(PROJECT_BACKGROUND_JOBS, "Detached user processes should have a different ref count");
    kthread->refCount = 2;

    kthread->alive = true;
    Clear_Thread_Queue(&kthread->joinQueue);
    kthread->pid = nextFreePid++;

}

/*
 * Create a new raw thread object.
 * Returns a null pointer if there isn't enough memory.
 */
static struct Kernel_Thread *Create_Thread(int priority) {
    struct Kernel_Thread *kthread;
    void *stackPage = 0;

    /*
     * For now, just allocate one page each for the thread context
     * object and the thread's stack.
     */
    kthread = Alloc_Page();
    if (kthread != 0)
        stackPage = Alloc_Page();

    /* Make sure that the memory allocations succeeded. */
    if (kthread == 0)
        return 0;
    if (stackPage == 0) {
        Free_Page(kthread);
        return 0;
    }

    /*Print("New thread @ %x, stack @ %x\n", kthread, stackPage); */

    /*
     * Initialize the stack pointer of the new thread
     * and accounting info
     */
    Init_Thread(kthread, stackPage, priority);

    /* Add to the list of all threads in the system. */
    Add_To_Back_Of_All_Thread_List(&s_allThreadList, kthread);

    return kthread;
}

/*
 * Push a dword value on the stack of given thread.
 * We use this to set up some context for the thread before
 * we make it runnable.
 */
static __inline__ void Push(struct Kernel_Thread *kthread, ulong_t value) {
    kthread->esp -= 4;
    *((ulong_t *) kthread->esp) = value;
}

/*
 * Destroy given thread.
 * This function should perform all cleanup needed to
 * reclaim the resources used by a thread.
 * Called with interrupts enabled.
 */
static void Destroy_Thread(struct Kernel_Thread *kthread) {
    /*
     * Detach the thread's user context, if any.
     * This will reclaim pages used by user processes.
     */
    if (kthread->userContext != 0)
        Detach_User_Context(kthread);

    /* Dispose of the thread's memory. */
    Disable_Interrupts();
    Free_Page(kthread->stackPage);
    Free_Page(kthread);

    /* Remove from list of all threads */
    Remove_From_All_Thread_List(&s_allThreadList, kthread);

    Enable_Interrupts();

}

/*
 * Hand given thread to the reaper for destruction.
 * Must be called with interrupts disabled!
 */
static void Reap_Thread(struct Kernel_Thread *kthread) {
    KASSERT(!Interrupts_Enabled());
    Enqueue_Thread(&s_graveyardQueue, kthread);
    Wake_Up(&s_reaperWaitQueue);
}

/*
 * Called when a reference to the thread is broken.
 */
static void Detach_Thread(struct Kernel_Thread *kthread) {
    KASSERT(!Interrupts_Enabled());
    KASSERT(kthread->refCount > 0);

    --kthread->refCount;
    if (kthread->refCount == 0) {
        Reap_Thread(kthread);
    }
}

/*
 * This function performs any needed initialization before
 * a thread start function is executed.  Currently we just use
 * it to enable interrupts (since Schedule() always activates
 * a thread with interrupts disabled).
 */
static void Launch_Thread(void) {
    Enable_Interrupts();
}

/*
 * Push initial values for general purpose registers.
 */
static void Push_General_Registers(struct Kernel_Thread *kthread) {
    /*
     * Push initial values for saved general-purpose registers.
     * (The actual values are not important.)
     */
    Push(kthread, 0);           /* eax */
    Push(kthread, 0);           /* ebx */
    Push(kthread, 0);           /* edx */
    Push(kthread, 0);           /* edx */
    Push(kthread, 0);           /* esi */
    Push(kthread, 0);           /* edi */
    Push(kthread, 0);           /* ebp */
}

/*
 * Shutdown a kernel thread.
 * This is called if a kernel thread exits by falling off
 * the end of its start function.
 */
static void Shutdown_Thread(void) {
    Exit(0);
}

/*
 * Set up the initial context for a kernel-mode-only thread.
 */
static void Setup_Kernel_Thread(struct Kernel_Thread *kthread,
                                Thread_Start_Func startFunc, ulong_t arg) {
    /*
     * Push the argument to the thread start function, and the
     * return address (the Shutdown_Thread function, so the thread will
     * go away cleanly when the start function returns).
     */
    Push(kthread, arg);
    Push(kthread, (ulong_t) & Shutdown_Thread);

    /* Push the address of the start function. */
    Push(kthread, (ulong_t) startFunc);

    /*
     * To make the thread schedulable, we need to make it look
     * like it was suspended by an interrupt.  This means pushing
     * an "eflags, cs, eip" sequence onto the stack,
     * as well as int num, error code, saved registers, etc.
     */

    /*
     * The EFLAGS register will have all bits clear.
     * The important constraint is that we want to have the IF
     * bit clear, so that interrupts are disabled when the
     * thread starts.
     */
    Push(kthread, 0UL);         /* EFLAGS */

    /*
     * As the "return address" specifying where the new thread will
     * start executing, use the Launch_Thread() function.
     */
    Push(kthread, KERNEL_CS);
    Push(kthread, (ulong_t) & Launch_Thread);

    /* Push fake error code and interrupt number. */
    Push(kthread, 0);
    Push(kthread, 0);

    /* Push initial values for general-purpose registers. */
    Push_General_Registers(kthread);

    /*
     * Push values for saved segment registers.
     * Only the ds and es registers will contain valid selectors.
     * The fs and gs registers are not used by any instruction
     * generated by gcc.
     */
    Push(kthread, KERNEL_DS);   /* ds */
    Push(kthread, KERNEL_DS);   /* es */
    Push(kthread, 0);           /* fs */
    Push(kthread, 0);           /* gs */
}

/*
 * Set up the a user mode thread.
 */
/*static*/ void Setup_User_Thread(
                                     struct Kernel_Thread *kthread,
                                     struct User_Context *userContext) {
    extern int userDebug;

    /*
     * Interrupts in user mode MUST be enabled.
     * All other EFLAGS bits will be clear.
     */
    ulong_t eflags = EFLAGS_IF;

    unsigned csSelector = userContext->csSelector;
    unsigned dsSelector = userContext->dsSelector;

    Attach_User_Context(kthread, userContext);

    /*
     * Make the thread's stack look like it was interrupted
     * while in user mode.
     */

    /* Stack segment and stack pointer within user mode. */
    Push(kthread, dsSelector);  /* user ss */
    Push(kthread, userContext->stackPointerAddr);       /* user esp */

    /* eflags, cs, eip */
    Push(kthread, eflags);
    Push(kthread, csSelector);
    Push(kthread, userContext->entryAddr);
    if (userDebug)
        Print("Entry addr=%lx\n", userContext->entryAddr);

    /* Push fake error code and interrupt number. */
    Push(kthread, 0);
    Push(kthread, 0);

    /*
     * Push initial values for general-purpose registers.
     * The only important register is esi, which we use to
     * pass the address of the argument block.
     */
    Push(kthread, 0);           /* eax */
    Push(kthread, 0);           /* ebx */
    Push(kthread, 0);           /* edx */
    Push(kthread, 0);           /* edx */
    Push(kthread, userContext->argBlockAddr);   /* esi */
    Push(kthread, 0);           /* edi */
    Push(kthread, 0);           /* ebp */

    /* Push initial values for the data segment registers. */
    Push(kthread, dsSelector);  /* ds */
    Push(kthread, dsSelector);  /* es */
    Push(kthread, dsSelector);  /* fs */
    Push(kthread, dsSelector);  /* gs */
}


/*
 * This is the body of the idle thread.  Its job is to preserve
 * the invariant that a runnable thread always exists,
 * i.e., the run queue is never empty.
 */
static void Idle(ulong_t arg __attribute__ ((unused))) {
    while (true) {
        /* 
         * The hlt instruction tells the CPU to wait until an interrupt is called.
         * We call this in this loop so the Idle process does not eat up 100% cpu,
         * and make our laptops catch fire.
         */
        __asm__("hlt");
        Yield();
    }
}

/*
 * The reaper thread.  Its job is to de-allocate memory
 * used by threads which have finished.
 */
static void Reaper(ulong_t arg __attribute__ ((unused))) {
    struct Kernel_Thread *kthread;

    Disable_Interrupts();

    while (true) {
        /* See if there are any threads needing disposal. */
        if ((kthread = s_graveyardQueue.head) == 0) {
            /* Graveyard is empty, so wait for a thread to die. */
            Wait(&s_reaperWaitQueue);
        } else {
            /* Make the graveyard queue empty. */
            Clear_Thread_Queue(&s_graveyardQueue);

            /*
             * Now we can re-enable interrupts, since we
             * have removed all the threads needing disposal.
             */
            Enable_Interrupts();
            Yield();            /* allow other threads to run? */

            /* Dispose of the dead threads. */
            while (kthread != 0) {
                struct Kernel_Thread *next =
                    Get_Next_In_Thread_Queue(kthread);
#if 0
                Print("Reaper: disposing of thread @ %x, stack @ %x\n",
                      kthread, kthread->stackPage);
#endif
                Destroy_Thread(kthread);
                kthread = next;
            }

            /*
             * Disable interrupts again, since we're going to
             * do another iteration.
             */
            Disable_Interrupts();
        }
    }
}

/*
 * Find the best (highest priority) thread in given
 * thread queue.  Returns null if queue is empty.
 */
static __inline__ struct Kernel_Thread *Find_Best(struct Thread_Queue *queue) {
    /* Pick the highest priority thread */
    struct Kernel_Thread *kthread = queue->head, *best = 0;
    while (kthread != 0) {
        if (best == 0 || kthread->priority > best->priority)
            best = kthread;
        kthread = Get_Next_In_Thread_Queue(kthread);
    }
    return best;
}


/*
 * Acquires pointer to thread-local data from the current thread
 * indexed by the given key.  Assumes interrupts are off.
 */
static __inline__ const void **Get_Tlocal_Pointer(tlocal_key_t k) {
    struct Kernel_Thread *current = g_currentThread;

    KASSERT(k < MAX_TLOCAL_KEYS);

    return &current->tlocalData[k];
}

/*
 * Clean up any thread-local data upon thread exit.  Assumes
 * this is called with interrupts disabled.  We follow the POSIX style
 * of possibly invoking a destructor more than once, because a
 * destructor to some thread-local data might cause other thread-local
 * data to become alive once again.  If everything is NULL by the end
 * of an iteration, we are done.
 */
static void Tlocal_Exit(struct Kernel_Thread *curr) {
    int i, j, called = 0;

    KASSERT(!Interrupts_Enabled());

    for (j = 0; j < MIN_DESTRUCTOR_ITERATIONS; j++) {

        for (i = 0; i < MAX_TLOCAL_KEYS; i++) {

            void *x = (void *)curr->tlocalData[i];
            if (x != NULL && s_tlocalDestructors[i] != NULL) {

                curr->tlocalData[i] = NULL;
                called = 1;

                Enable_Interrupts();
                s_tlocalDestructors[i] (x);
                Disable_Interrupts();
            }
        }
        if (!called)
            break;
    }
}


/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

void Init_Scheduler(void) {
    struct Kernel_Thread *mainThread =
        (struct Kernel_Thread *)KERN_THREAD_OBJ;

    /*
     * Create initial kernel thread context object and stack,
     * and make them current.
     */
    Init_Thread(mainThread, (void *)KERN_STACK, PRIORITY_NORMAL);
    g_currentThread = mainThread;
    Add_To_Back_Of_All_Thread_List(&s_allThreadList, mainThread);

    /*
     * Create the idle thread.
     */
    /*Print("starting idle thread\n"); */
    Start_Kernel_Thread(Idle, 0, PRIORITY_IDLE);

    /*
     * Create the reaper thread.
     */
    /*Print("starting reaper thread\n"); */
    Start_Kernel_Thread(Reaper, 0, PRIORITY_NORMAL);
}

/*
 * Start a kernel-mode-only thread, using given function as its body
 * and passing given argument as its parameter.  Returns pointer
 * to the new thread if successful, null otherwise.
 *
 * startFunc - is the function to be called by the new thread
 * arg - is a paramter to pass to the new function
 * priority - the priority of this thread (use PRIORITY_NORMAL) for
 *    most things
 */
struct Kernel_Thread *Start_Kernel_Thread(Thread_Start_Func startFunc,
                                          ulong_t arg,
                                          int priority) {
    struct Kernel_Thread *kthread = Create_Thread(priority);
    if (kthread != 0) {
        /*
         * Create the initial context for the thread to make
         * it schedulable.
         */
        Setup_Kernel_Thread(kthread, startFunc, arg);


        /* Atomically put the thread on the run queue. */
        Make_Runnable_Atomic(kthread);
    }

    return kthread;
}

/*
 * Start a user-mode thread (i.e., a process), using given user context.
 * Returns pointer to the new thread if successful, null otherwise.
 */
struct Kernel_Thread *Start_User_Thread(struct User_Context *userContext) {
    struct Kernel_Thread *kthread = Create_Thread(PRIORITY_USER);
    if (kthread != 0) {
        /* Set up the thread, and put it on the run queue */
        Setup_User_Thread(kthread, userContext);
        Make_Runnable_Atomic(kthread);
    }

    return kthread;
}

/*
 * Add given thread to the run queue, so that it
 * may be scheduled.  Must be called with interrupts disabled!
 */
void Make_Runnable(struct Kernel_Thread *kthread) {
    KASSERT(!Interrupts_Enabled());

    Enqueue_Thread(&s_runQueue, kthread);
}

/*
 * Atomically make a thread runnable.
 * Assumes interrupts are currently enabled.
 */
void Make_Runnable_Atomic(struct Kernel_Thread *kthread) {
    Disable_Interrupts();
    Make_Runnable(kthread);
    Enable_Interrupts();
}

/*
 * Get the thread that currently has the CPU.
 */
struct Kernel_Thread *Get_Current(void) {
    return g_currentThread;
}

/*
 * Get the next runnable thread from the run queue.
 * This is the scheduler.
 */
struct Kernel_Thread *Get_Next_Runnable(void) {
    struct Kernel_Thread *best = 0;

    best = Find_Best(&s_runQueue);
    KASSERT(best != 0);
    Remove_Thread(&s_runQueue, best);

/*
 *    Print("Scheduling %x\n", best);
 */
    return best;
}

/*
 * Schedule a thread that is waiting to run.
 * Must be called with interrupts off!
 * The current thread should already have been placed
 * on whatever queue is appropriate (i.e., either the
 * run queue if it is still runnable, or a wait queue
 * if it is waiting for an event to occur).
 */
void Schedule(void) {
    struct Kernel_Thread *runnable;

    /* Make sure interrupts really are disabled */
    KASSERT(!Interrupts_Enabled());

    /* Preemption should not be disabled. */
    KASSERT(!g_preemptionDisabled);

    /* Get next thread to run from the run queue */
    runnable = Get_Next_Runnable();

    /*
     * Activate the new thread, saving the context of the current thread.
     * Eventually, this thread will get re-activated and Switch_To_Thread()
     * will "return", and then Schedule() will return to wherever
     * it was called from.
     */
    Switch_To_Thread(runnable);
}

/*
 * Voluntarily give up the CPU to another thread.
 * Does nothing if no other threads are ready to run.
 */
void Yield(void) {
    Disable_Interrupts();
    Make_Runnable(g_currentThread);
    Schedule();
    Enable_Interrupts();
}


/*
 * Exit the current thread.
 * Calling this function initiates a context switch.
 */
void Exit(int exitCode) {
    struct Kernel_Thread *current = g_currentThread;

    if (Interrupts_Enabled())
        Disable_Interrupts();

    /* Thread is dead */
    current->exitCode = exitCode;
    current->alive = false;

    /* Remove timer references this thread has held */
    Alarm_Cancel_For_Thread(current);

    /* Clean up any thread-local memory */
    Tlocal_Exit(g_currentThread);

    /* Notify the thread's owner, if any */
    Wake_Up(&current->joinQueue);

    /* Remove the thread's implicit reference to itself. */
    Detach_Thread(g_currentThread);

    /*
     * Schedule a new thread.
     * Since the old thread wasn't placed on any
     * thread queue, it won't get scheduled again.
     */
    Schedule();

    /* Shouldn't get here */
    KASSERT(false);
}

/*
 * Wait for given thread to die.
 * Interrupts must be enabled.
 * Returns the thread exit code.
 */
int Join(struct Kernel_Thread *kthread) {
    int exitCode;

    KASSERT(Interrupts_Enabled());

    /* It is only legal for the owner to join */
    KASSERT(kthread->owner == g_currentThread);

    Disable_Interrupts();

    /* Wait for it to die */
    while (kthread->alive) {
        Wait(&kthread->joinQueue);
    }

    /* Get thread exit code. */
    exitCode = kthread->exitCode;

    /* Release our reference to the thread */
    Detach_Thread(kthread);

    Enable_Interrupts();

    return exitCode;
}


/*
 * Look up a thread by its process id.
 * notOwner is true if the thread is assumed to be the caller's
 * owner.  If the thread is NOT the owner, then it is assumed
 * the caller will not retain the extra reference with interrupts
 * enabled, or else the thread could die and create a dangling pointer.
 */
struct Kernel_Thread *Lookup_Thread(int pid, int notOwner) {
    struct Kernel_Thread *result = 0;

    bool iflag = Begin_Int_Atomic();

    /*
     * TODO: we could remove the requirement that the caller
     * needs to be the thread's owner by specifying that another
     * reference is added to the thread before it is returned.
     */

    result = Get_Front_Of_All_Thread_List(&s_allThreadList);
    while (result != 0) {
        if (result->pid == pid) {
            if (g_currentThread != result->owner && !notOwner)
                result = 0;
            break;
        }
        result = Get_Next_In_All_Thread_List(result);
    }

    End_Int_Atomic(iflag);

    return result;
}

/*
 * Wait on given wait queue.
 * Must be called with interrupts disabled!
 * Note that the function will return with interrupts
 * disabled.  This is desirable, because it allows us to
 * atomically test a condition that can be affected by an interrupt
 * and wait for it to be satisfied (if necessary).
 * See the Wait_For_Key() function in keyboard.c
 * for an example.
 */

void Wait(struct Thread_Queue *waitQueue) {
    struct Kernel_Thread *current = g_currentThread;

    KASSERT(!Interrupts_Enabled());
    KASSERT(waitQueue != NULL); /* try to protect against passing uninitialized pointers in */

    /* Add the thread to the wait queue. */
    Enqueue_Thread(waitQueue, current);

    /* Find another thread to run. */
    Schedule();

}

/*
 * Wake up all threads waiting on given wait queue.
 * Must be called with interrupts disabled!
 * See Keyboard_Interrupt_Handler() function in keyboard.c
 * for an example.
 */
void Wake_Up(struct Thread_Queue *waitQueue) {
    struct Kernel_Thread *kthread = waitQueue->head, *next;

    KASSERT(!Interrupts_Enabled());
    KASSERT(waitQueue != NULL); /* try to protect against passing uninitialized pointers in */

    /*
     * Walk throught the list of threads in the wait queue,
     * transferring each one to the run queue.
     */
    for (kthread = waitQueue->head; kthread != 0; kthread = next) {
        next = Get_Next_In_Thread_Queue(kthread);
        Make_Runnable(kthread);
    }

    /* The wait queue is now empty. */
    Clear_Thread_Queue(waitQueue);
}



/*
 * Wake up a single thread waiting on given wait queue
 * (if there are any threads waiting).  Chooses the highest priority thread.
 * Interrupts must be disabled!
 */
void Wake_Up_One(struct Thread_Queue *waitQueue) {
    struct Kernel_Thread *best;

    KASSERT(!Interrupts_Enabled());
    KASSERT(waitQueue != NULL); /* try to protect against passing uninitialized pointers in */

    best = Find_Best(waitQueue);

    if (best != 0) {
        Remove_Thread(waitQueue, best);
        Make_Runnable(best);
        /*Print("Wake_Up_One: waking up %x from %x\n", best, g_currentThread); */
    }
}

/*
 * Allocate a key for accessing thread-local data.
 */
int Tlocal_Create(tlocal_key_t * key, tlocal_destructor_t destructor) {
    bool iflag = Begin_Int_Atomic();

    KASSERT(key);

    if (s_tlocalKeyCounter == MAX_TLOCAL_KEYS)
        return -1;
    s_tlocalDestructors[s_tlocalKeyCounter] = destructor;
    *key = s_tlocalKeyCounter++;

    End_Int_Atomic(iflag);

    return 0;
}

/*
 * Store a value for a thread-local item
 */
void Tlocal_Put(tlocal_key_t k, const void *v) {
    const void **pv;

    KASSERT(k < s_tlocalKeyCounter);

    pv = Get_Tlocal_Pointer(k);
    *pv = v;
}

/*
 * Acquire a thread-local value
 */
void *Tlocal_Get(tlocal_key_t k) {
    const void **pv;

    KASSERT(k < s_tlocalKeyCounter);

    pv = Get_Tlocal_Pointer(k);
    return (void *)*pv;
}

/*
 * Print list of all threads in system.
 * For debugging.
 */
void Dump_All_Thread_List(void) {
    struct Kernel_Thread *kthread;
    int count = 0;
    bool iflag = Begin_Int_Atomic();

    kthread = Get_Front_Of_All_Thread_List(&s_allThreadList);

    Print("[");
    while (kthread != 0) {
        ++count;
        Print("<%lx,%lx,%lx>",
              (ulong_t) Get_Prev_In_All_Thread_List(kthread),
              (ulong_t) kthread,
              (ulong_t) Get_Next_In_All_Thread_List(kthread));
        KASSERT(kthread != Get_Next_In_All_Thread_List(kthread));
        kthread = Get_Next_In_All_Thread_List(kthread);
    }
    Print("]\n");
    Print("%d threads are running\n", count);

    End_Int_Atomic(iflag);
}
