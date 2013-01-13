#include <limits.h>
#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/bitset.h>
#include <geekos/synch.h>
#include <geekos/bufcache.h>
#include <geekos/list.h>
#include <geekos/gosfs.h>
#include <geekos/vfs.h>
#include <geekos/string.h>


/*
 * Format a drive with GOSFS.
 */
int GOSFS_Format(struct Block_Device *blockDev) {
    TODO("GeekOS file system Format operation");
}

/*
 * Mount GOSFS. Return 0 on success, return < 0 on failure.
 * - Check that the magic number is correct.
 */
int GOSFS_Mount(struct Mount_Point *mountPoint) {
    TODO("GeekOS file system Mount operation");
}

/*
 * Get metadata for given File. Called with a file descriptor.
 */
int GOSFS_FStat(struct File *file, struct VFS_File_Stat *stat) {
    TODO("GeekOS file system FStat operation");
}

/*
 * Open a file with the given name and mode.
 * Return > 0 on success, < 0 on failure (e.g. does not exist).
 */
int GOSFS_Open(struct Mount_Point *mountPoint, const char *path, int mode,
               struct File **pFile) {
    TODO("GeekOS file system Open operation");
}

/*
 * Read data from current position in file.
 * Return > 0 on success, < 0 on failure.
 */
int GOSFS_Read(struct File *file, void *buf, ulong_t numBytes) {
    TODO("GeekOS file system Read operation");
}

/*
 * Write data to current position in file.
 * Return > 0 on success, < 0 on failure.
 */
int GOSFS_Write(struct File *file, void *buf, ulong_t numBytes) {
    TODO("GeekOS file system Write operation");
}

/*
 * Get metadata for given file. Need to find the file from the given path.
 */
int GOSFS_Stat(struct Mount_Point *mountPoint, const char *path,
               struct VFS_File_Stat *stat) {
    TODO("GeekOS file system Stat operation");
}

/*
 * Synchronize the filesystem data with the disk
 * (i.e., flush out all buffered filesystem data).
 */
int GOSFS_Sync(struct Mount_Point *mountPoint) {
    TODO("GeekOS file system Sync operation");
}

/*
 * Close a file.
 */
int GOSFS_Close(struct File *file) {
    TODO("GeekOS file system Close operation");
}

/*
 * Create a directory named by given path.
 */
int GOSFS_Create_Directory(struct Mount_Point *mountPoint, const char *path) {
    TODO("GeekOS file system Create Directory operation");
}

/*
 * Open a directory named by given path.
 */
int GOSFS_Open_Directory(struct Mount_Point *mountPoint, const char *path,
                         struct File **pDir) {
    TODO("GeekOS file system Open Directory operation");
}

/*
 * Seek to a position in file. Should not seek beyond the end of the file.
 */
int GOSFS_Seek(struct File *file, ulong_t pos) {
    TODO("GeekOS file system Seek operation");
}

/*
 * Delete the given file.
 * Return > 0 on success, < 0 on failure.
 */
int GOSFS_Delete(struct Mount_Point *mountPoint, const char *path) {
    TODO("GeekOS file system Delete operation");
}

/*
 * Read a directory entry from an open directory.
 * Return > 0 on success, < 0 on failure.
 */
int GOSFS_Read_Entry(struct File *dir, struct VFS_Dir_Entry *entry) {
    TODO("GeekOS file system Read Directory operation");
}

static struct Filesystem_Ops s_gosfsFilesystemOps = {
    &GOSFS_Format,
    &GOSFS_Mount,
};

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

void Init_GOSFS(void) {
    Register_Filesystem("gosfs", &s_gosfsFilesystemOps);
}
