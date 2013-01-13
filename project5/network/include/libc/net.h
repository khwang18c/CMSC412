/*
 * Network
 * Copyright (c) 2009, Calvin Grunewald
 * $Revision: 1.0 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef _LIBC_NET_H_
#define _LIBC_NET_H_

#include <geekos/defs.h>
#include <geekos/ktypes.h>

#define ETH_MAX_DATA 1500
#define ETH_MIN_DATA 46


int EthPacketSend(const void *buffer, ulong_t length, const uchar_t dest[],
                  const char *device);
int EthPacketReceive(void *buffer, ulong_t length);
int Arp(uchar_t *, uchar_t *);

#endif
