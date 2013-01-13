/*
 * format - Format a filesystem on a block device
 * Copyright (c) 2008, Aaron Schulman <schulman@cs.umd.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <process.h>
#include <fileio.h>
#include <geekos/gfs2.h>

int main(int argc, char *argv[]) {
    int rc = -1;

    if (argc != 4) {
        Print("Usage: gfs2f <devname> <size MB> <block size KB>\n");
        Exit(-1);
    }

    /* 
     * TODO Use the ReadBlock and WriteBlock system calls to write to the disk
     * ex. ReadBlock("ide1", buf, sizeof(buf));
     *
     * You will have to implement the ReadBlock and WriteBlock system calls in the kernel.
     *
     */

    if (rc != 0) {
        Print("Error: Could not format gfs2 on %s\n", argv[1]);
        Exit(-1);
    }

    return 0;
}
