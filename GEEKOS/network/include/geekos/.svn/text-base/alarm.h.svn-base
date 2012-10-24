/*
 * Alarm / timer support
 * Copyright (c) 2009 Calvin Grunewald
 * $Revision: 1.00 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef ALARM_H
#define ALARM_H

#include <geekos/defs.h>
#include <geekos/ktypes.h>
#include <geekos/list.h>

typedef void (*Alarm_Callback) (void *);

struct Alarm_Event;

DEFINE_LIST(Alarm_Handler_Queue, Alarm_Event);

struct Alarm_Event {
    int timerId;
    Alarm_Callback callback;
    void *data;
    struct Kernel_Thread *thread;

     DEFINE_LINK(Alarm_Handler_Queue, Alarm_Event);
};

int Alarm_Cancel_For_Thread(struct Kernel_Thread *thread);
int Alarm_Create(Alarm_Callback callback, void *data,
                 unsigned int milliSeconds);
int Alarm_Destroy(int id);
void Init_Alarm(void);

IMPLEMENT_LIST(Alarm_Handler_Queue, Alarm_Event);

#endif
