/*
 * GeekOS file system
 * Copyright (c) 2008, David H. Hovemeyer <daveho@cs.umd.edu>, 
 * Neil Spring <nspring@cs.umd.edu>, Aaron Schulman <schulman@cs.umd.edu>
 * $Revision: 1.56 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <limits.h>
#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/bitset.h>
#include <geekos/synch.h>
#include <geekos/bufcache.h>
#include <geekos/gfs2.h>

/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
 * Implementation of VFS operations
 * ---------------------------------------------------------------------- */

/*
 * Get metadata for given file.
 */
static int GFS2_FStat(struct File *file, struct VFS_File_Stat *stat) {
    TODO("GeekOS filesystem FStat operation");
}

/*
 * Read data from current position in file.
 */
static int GFS2_Read(struct File *file, void *buf, ulong_t numBytes) {
    TODO("GeekOS filesystem read operation");
}

/*
 * Write data to current position in file.
 */
static int GFS2_Write(struct File *file, void *buf, ulong_t numBytes) {
    TODO("GeekOS filesystem write operation");
}

/*
 * Seek to a position in file.
 */
static int GFS2_Seek(struct File *file, ulong_t pos) {
    TODO("GeekOS filesystem seek operation");
}

/*
 * Close a file.
 */
static int GFS2_Close(struct File *file) {
    TODO("GeekOS filesystem close operation");
}

/*static*/ struct File_Ops s_gosfsFileOps = {
    &GFS2_FStat,
    &GFS2_Read,
    &GFS2_Write,
    &GFS2_Seek,
    &GFS2_Close,
    0,                          /* Read_Entry */
};

/*
 * Stat operation for an already open directory.
 */
static int GFS2_FStat_Directory(struct File *dir, struct VFS_File_Stat *stat) {
    TODO("GeekOS filesystem FStat directory operation");
}

/*
 * Directory Close operation.
 */
static int GFS2_Close_Directory(struct File *dir) {
    TODO("GeekOS filesystem Close directory operation");
}

/*
 * Read a directory entry from an open directory.
 */
static int GFS2_Read_Entry(struct File *dir, struct VFS_Dir_Entry *entry) {
    TODO("GeekOS filesystem Read_Entry operation");
}

/*static*/ struct File_Ops s_gfs2DirOps = {
    &GFS2_FStat_Directory,
    0,                          /* Read */
    0,                          /* Write */
    0,                          /* Seek */
    &GFS2_Close_Directory,
    &GFS2_Read_Entry,
};

/*
 * Open a file named by given path.
 */
static int GFS2_Open(struct Mount_Point *mountPoint, const char *path,
                     int mode, struct File **pFile) {
    TODO("GeekOS filesystem open operation");
}

/*
 * Create a directory named by given path.
 */
static int GFS2_Create_Directory(struct Mount_Point *mountPoint,
                                 const char *path) {
    TODO("GeekOS filesystem create directory operation");
}

/*
 * Open a directory named by given path.
 */
static int GFS2_Open_Directory(struct Mount_Point *mountPoint,
                               const char *path, struct File **pDir) {
    TODO("GeekOS filesystem open directory operation");
}

/*
 * Open a directory named by given path.
 */
static int GFS2_Delete(struct Mount_Point *mountPoint, const char *path) {
    TODO("GeekOS filesystem delete operation");
}

/*
 * Get metadata (size, permissions, etc.) of file named by given path.
 */
static int GFS2_Stat(struct Mount_Point *mountPoint, const char *path,
                     struct VFS_File_Stat *stat) {
    TODO("GeekOS filesystem stat operation");
}

/*
 * Synchronize the filesystem data with the disk
 * (i.e., flush out all buffered filesystem data).
 */
static int GFS2_Sync(struct Mount_Point *mountPoint) {
    TODO("GeekOS filesystem sync operation");
}

/*static*/ struct Mount_Point_Ops s_gfs2MountPointOps = {
    &GFS2_Open,
    &GFS2_Create_Directory,
    &GFS2_Open_Directory,
    &GFS2_Stat,
    &GFS2_Sync,
    &GFS2_Delete,
};

static int GFS2_Format(struct Block_Device *blockDev) {
    TODO("DO NOT IMPLEMENT: There is no format operation for GFS2");
}

static int GFS2_Mount(struct Mount_Point *mountPoint) {
    TODO("GeekOS filesystem mount operation");
}

static struct Filesystem_Ops s_gfs2FilesystemOps = {
    &GFS2_Format,
    &GFS2_Mount,
};

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

void Init_GFS2(void) {
    Register_Filesystem("gfs2", &s_gfs2FilesystemOps);
}
