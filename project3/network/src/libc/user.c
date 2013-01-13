/*
 * User ids
 * Copyright (c) 2011, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * $Revision: 1.12 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/syscall.h>
#include <geekos/sem.h>
#include <string.h>

DEF_SYSCALL(GetUid,SYS_GET_UID,int,(), , SYSCALL_REGS_0)
DEF_SYSCALL(SetSetUid,SYS_SET_SET_UID,int,(const char *filename, int setuid),
    const char *arg0 = filename; size_t arg1 = strlen(filename); int arg2 = setuid;,
    SYSCALL_REGS_3)
DEF_SYSCALL(SetEffectiveUid,SYS_SET_EFFECTIVE_UID,int,(int uid),
    int arg0 = uid;,
    SYSCALL_REGS_1)
DEF_SYSCALL(SetAcl,SYS_SET_ACL,int,(const char *filename, int uid, int permissions),
    const char *arg0 = filename; size_t arg1 = strlen(filename); int arg2 = uid; int arg3 = permissions;,
    SYSCALL_REGS_4)

