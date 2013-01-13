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
#include <geekos/defs.h>

#define DISK_BLOCKS_PER_GOSFS 8
#define GOSFS_MAGIC 0xBEEBEE // GOSFS magic number
#define SECTOR_SIZE 512
#define MAX_NAME_SIZE 64
#define INT_ARR_SIZE 1024
#define SINGLE_BLOCK 8
#define DBL_BLOCK 9
#define SINGLE_MAX_INDEX 1032
#define SINGLE_MIN_INDEX 7

/* ----------------------------------------------------------------------
 * Different operations allowed for mountpoints, files, and directorys
 * ---------------------------------------------------------------------- */

/*
 * Mount_Point_Ops for GOSFS filesystem
 */
static struct Mount_Point_Ops s_gosfsMountPointOps = {  
    &GOSFS_Open,
    &GOSFS_Create_Directory,
    &GOSFS_Open_Directory,
    &GOSFS_Stat,
    &GOSFS_Sync,
    &GOSFS_Delete,
    0,              /* SetSetUid */
    0               /* SetAcle */
};

/*
 * File_Ops for GOSFS files
 */
static struct File_Ops s_gosfsFileOps = {
    &GOSFS_FStat,
    &GOSFS_Read,
    &GOSFS_Write,
    &GOSFS_Seek,
    &GOSFS_Close,
    0              /* Read_Entry */
};

/*
 *  File_Ops for GOSFS directory
 */
static struct File_Ops s_gosfsDirOps = {
    &GOSFS_FStat,              /* Fstat */ 
    &GOSFS_Read,              /* Read */
    0,              /* Write */
    &GOSFS_Seek,              /* Seek */
    &GOSFS_Close,              /* Close */
    &GOSFS_Read_Entry
};
      
/* ----------------------------------------------------------------------
 * Helper functions
 * ---------------------------------------------------------------------- */

/* 
 * Write a full GOSFS block to disk
 * Return 0 on success, exits on error
 * Note: blocknum is the GOSFS block number
 */
int GOSFS_Block_Write(struct Block_Device *dev, int blockNum, void *buf) {
    int rc = 0;
    int index = 0; 
    for (index = 0; index < DISK_BLOCKS_PER_GOSFS; index++) {
        rc = Block_Write(dev, blockNum * DISK_BLOCKS_PER_GOSFS + index, 
                         buf + (index * SECTOR_SIZE)); 
        if (rc != 0) 
            Exit(rc);
    }
        
    return rc;
}

/*
 * Read a full GOSFS block from disk
 * Return 0 on success, exits on error
 * Note: blocknum is the GOSFS block number
 */
int GOSFS_Block_Read(struct Block_Device *dev, int blockNum, void *buf) {
    int rc = 0;
    int index = 0; 
    for (index = 0; index < DISK_BLOCKS_PER_GOSFS; index++) {
        rc = Block_Read(dev, blockNum * DISK_BLOCKS_PER_GOSFS + index, 
                         buf + (index * SECTOR_SIZE)); 
        if (rc != 0)
            Exit(rc);
    }
        
    return rc;
}

/*
 * Searches through each fileNode entry in dir for a node that matches name.
 * blockNum is the GOSFS block number dir resides in, and meta is the GOSFSptr 
 * that we will be filling with all the data we need to know about the file/dir.
 * Returns true if entry found, else false. 
 */
bool GOSFS_Lookup(GOSFSdirectory *dir, const char *name, int blockNum, GOSFSptr *entry) {
    int rc = ENOTFOUND;
    int index = 0;
    GOSFSfileNode *currNode = 0;

    for (index = 0; index < MAX_FILES_PER_DIR; index++) {
        currNode = &dir->files[index];           
        if (currNode->isUsed == 1 && strcmp(currNode->name, name) == 0)  {
            memcpy(&entry->node, currNode, sizeof(GOSFSfileNode));; 
            entry->blockNum = blockNum;
            entry->offset = index;
            return true;
        }
    }

    return false; 
}

/* 
 * Copies the next dir/file name in the path into the param name.
 * Also removes the returned name from the path.
 */
void Get_Next_Name_In_Path(char **path, char *name) {
    int nameSize = 0;
    char *slashAddr = 0;
    char *newPath = &(*path)[1]; // Ignore first slash 
    
    /* Find index of the slash */
    slashAddr = strchr(newPath, '/');
    if (slashAddr == 0)
        nameSize = strlen(newPath);
    else
        nameSize = slashAddr - newPath;

    /* Copy name in path to name param */
    strncpy(name, newPath, nameSize);

    /* Moves path pointer to point to address after the name that is returned */
    *path = &newPath[nameSize]; 
}

/*
 * Creates a new filenode and store info in param data. Emulates changes in curr dir.
 * @param dir - the directory the filenode will be created in
 * @param name - the name of the file we are creating
 * @param blockNum - the GOSFS block number the directory is located in
 * @param entry - GOSFSptr that will hold all data I need to know about the file
 * @return 0 on success, < 0 on error
 */
int Create_File(GOSFSdirectory *dir, char *name, int blockNum, GOSFSptr *entry) {
    GOSFSfileNode *currNode = 0; 
    int index = 0;

    for (index = 0; index < MAX_FILES_PER_DIR; index++) {
        currNode = &dir->files[index];
        if (currNode->isUsed == 0) {
            /* Setup new GOSFSfileNode inside of the current direcotry */
            memcpy(currNode->name, name, MAX_NAME_SIZE);
            currNode->size = 0;
            currNode->isUsed = 1;
            currNode->isDirectory = 0;

            /* Setup GOSFSptr */
            memcpy(&entry->node, currNode, sizeof(GOSFSfileNode));
            entry->blockNum = blockNum;
            entry->offset = index;

            return 0;
        }
    }
    return -1;
}

/*
 * Mallocs memory of size bytes and memsets all bits to 0.
 * Not exactly correct, shouldn't be exiting on failed Malloc but who cares.
 * Returns pointer to memory allocated on success, exits on error
 */
void *Safe_Calloc(ulong_t size) {
    void *rtn = Malloc(size); 
    if (rtn == 0)
        Exit(ENOMEM);
    memset(rtn, '\0', size);
    return rtn;
}

/* 
 * Copy file metadata from fileNode into struct VFS_File_Stat object 
 */
void Copy_Stat(struct VFS_File_Stat *stat, GOSFSfileNode *node) { 
    stat->size = node->size;
    stat->isDirectory = node->isDirectory;
    stat->isSetuid = node->isSetUid;
}

int Double_Indirect_Helper(struct Block_Device *dev, GOSFSsuperblock *super,
                            int *array, int index) {
    int blockNum = 0;
    int tempBlock = 0;
    int *arr = Safe_Calloc(PAGE_SIZE);
    int *empty = Safe_Calloc(PAGE_SIZE);

    if (array[index/INT_ARR_SIZE] == 0) {
        blockNum = 0;
        /* Allocate block for double indirect */
        tempBlock = Find_First_Free_Bit(&super->bitmap, super->size);
        Set_Bit(&super->bitmap, tempBlock);
        array[index/INT_ARR_SIZE] = tempBlock;
        GOSFS_Block_Write(dev, array[index/INT_ARR_SIZE], empty);
    } else {
        GOSFS_Block_Read(dev, array[index/INT_ARR_SIZE], arr);
        blockNum = arr[index % INT_ARR_SIZE];
    }
    Free(empty);
    Free(arr);
    return blockNum;
}

void Double_Indirect_Set_Helper(struct Block_Device *dev, GOSFSsuperblock *super,
                                int *array, int index, int blockNum) {
    int *arr = Safe_Calloc(PAGE_SIZE);
    int *empty = Safe_Calloc(PAGE_SIZE);
    int newBlock = 0;

    if (array[index/INT_ARR_SIZE] == 0) {
        newBlock = Find_First_Free_Bit(&super->bitmap, super->size);
        Set_Bit(&super->bitmap, newBlock);
        array[index/INT_ARR_SIZE] = newBlock;
        GOSFS_Block_Write(dev, newBlock, empty);
    } 

    GOSFS_Block_Read(dev, array[index/INT_ARR_SIZE], arr); 
    arr[index % INT_ARR_SIZE] = blockNum; 
    GOSFS_Block_Write(dev, blockNum, empty);
    GOSFS_Block_Write(dev, array[index/INT_ARR_SIZE], arr);

    Free(empty);
    Free(arr);
}

/*
 * Gets the next block num we need to write to. Also handles setting blocks 
 * array in the node to the new block num
 */
int Get_Blocknum_To_Write(struct File *file) {
    int blockNum = 0;
    int tempBlock = 0;
    int index = ((int)file->filePos) / PAGE_SIZE; 
    int *array = Safe_Calloc(PAGE_SIZE);
    void *empty = Safe_Calloc(PAGE_SIZE);
    GOSFSptr *entry = (GOSFSptr *)file->fsData;
    GOSFSfileNode *node = &entry->node;
    GOSFSsuperblock *super = (GOSFSsuperblock *)file->mountPoint->fsData;
    struct Block_Device *dev = file->mountPoint->dev;

    /* Return the block num we need to write to, or 0 if it doesn't exist */
    if (index < SINGLE_BLOCK) {
        blockNum = node->blocks[index];
    } else if (index > SINGLE_MIN_INDEX && index < SINGLE_MAX_INDEX) {
        if (node->blocks[SINGLE_BLOCK] == 0) {
            blockNum = 0;
            /* Allocate new block for single indirect */
            tempBlock = Find_First_Free_Bit(&super->bitmap, super->size); 
            Set_Bit(&super->bitmap, tempBlock);
            node->blocks[SINGLE_BLOCK] = tempBlock;
            GOSFS_Block_Write(dev, tempBlock, empty);
        } else {
            GOSFS_Block_Read(dev, node->blocks[SINGLE_BLOCK], array);
            blockNum = array[index - SINGLE_BLOCK];
        }
    } else {
        if (node->blocks[DBL_BLOCK] == 0) {
            blockNum = 0;
            /* Allocate new block for double indirect */
            tempBlock = Find_First_Free_Bit(&super->bitmap, super->size);
            Set_Bit(&super->bitmap, tempBlock);
            node->blocks[DBL_BLOCK] = tempBlock;
            GOSFS_Block_Write(dev, tempBlock, empty);
        } else {
            GOSFS_Block_Read(dev, node->blocks[DBL_BLOCK], array);
            blockNum = Double_Indirect_Helper(dev, super, array, index - SINGLE_MAX_INDEX);
            GOSFS_Block_Write(dev, node->blocks[DBL_BLOCK], array); // In case array changed 
        }
    }

    /* If it does not exist than we need to allocate a new block */
    if (blockNum == 0) {
        blockNum = Find_First_Free_Bit(&super->bitmap, super->size);
        Set_Bit(&super->bitmap, blockNum); // Set the super bit map
        if (index < SINGLE_BLOCK) {
            node->blocks[index] = blockNum;    
            GOSFS_Block_Write(dev, node->blocks[index], empty);
        } else if (index > SINGLE_MIN_INDEX && index < SINGLE_MAX_INDEX) {
            GOSFS_Block_Read(dev, node->blocks[SINGLE_BLOCK], array);
            array[index - SINGLE_BLOCK] = blockNum;
            GOSFS_Block_Write(dev, blockNum, empty); // Write empty array back
            GOSFS_Block_Write(dev, node->blocks[SINGLE_BLOCK], array); // Write change back
        } else {
            GOSFS_Block_Read(dev, node->blocks[DBL_BLOCK], array);
            Double_Indirect_Set_Helper(dev, super, array, index - SINGLE_MAX_INDEX, blockNum); 
            GOSFS_Block_Write(dev, node->blocks[DBL_BLOCK], array);
        }
    }

    Free(empty);
    Free(array);
    return blockNum;
}

/* 
 * Double indirect helper 
 */
int Double_Indirect_Read_Helper(struct Block_Device *dev, int *array, int index) {
    int blockNum = 0;
    int *arr = Safe_Calloc(PAGE_SIZE);
    
    if (array[index/INT_ARR_SIZE] == 0) {
        goto done;
    } else {
        GOSFS_Block_Read(dev, array[index/INT_ARR_SIZE], arr); 
        blockNum = arr[index % INT_ARR_SIZE];
        goto done;
    }

  done:
    Free(arr);
    return blockNum;
}

/*
 * Returns the blocknum we are going to read from disk 
 */

int Get_Blocknum_To_Read(struct File *file) {
    int blockNum = 0;
    int index = ((int)file->filePos) / PAGE_SIZE; 
    int *array = Safe_Calloc(PAGE_SIZE);
    GOSFSptr *entry = (GOSFSptr *)file->fsData;
    GOSFSfileNode *node = &entry->node;
    struct Block_Device *dev = file->mountPoint->dev;

    if (index < SINGLE_BLOCK) {
        blockNum = node->blocks[index];
    } else if (index > SINGLE_MIN_INDEX && index < SINGLE_MAX_INDEX) {
        if (node->blocks[SINGLE_BLOCK] == 0) {
            blockNum = 0;
        } else {
            GOSFS_Block_Read(dev, node->blocks[SINGLE_BLOCK], array);
            blockNum = array[index - SINGLE_BLOCK];
        } 
    } else {
        if (node->blocks[DBL_BLOCK] == 0) {
            blockNum = 0;
        } else {
            GOSFS_Block_Read(dev, node->blocks[DBL_BLOCK], array);
            blockNum = Double_Indirect_Read_Helper(dev, array, index - SINGLE_MAX_INDEX);
        }
    }
   
    Free(array);
    return blockNum;
}

/*
 * Clears all of the bits in the superblock used for the file
 */
void Clear_File_Bits(void *bitmap, struct Block_Device *dev, GOSFSptr *entry) {
    int index = 0;
    int newIndex = 0;
    GOSFSfileNode *node = &entry->node;
    int *array = Safe_Calloc(PAGE_SIZE);
    int *array2 = Safe_Calloc(PAGE_SIZE);

    for (index = 0; index < (node->size/PAGE_SIZE); index++) {
        memset(array, '\0', PAGE_SIZE);
        memset(array2, '\0', PAGE_SIZE);
        if (index < SINGLE_BLOCK) {
            Clear_Bit(bitmap, node->blocks[index]);
        } else if (index > SINGLE_MIN_INDEX && index < SINGLE_MAX_INDEX) {
            GOSFS_Block_Read(dev, node->blocks[SINGLE_BLOCK], array);
            Clear_Bit(bitmap, array[index - SINGLE_BLOCK]);
            Clear_Bit(bitmap, node->blocks[SINGLE_BLOCK]);
        } else {
            GOSFS_Block_Read(dev, node->blocks[DBL_BLOCK], array);
            newIndex = index - SINGLE_MAX_INDEX;
            GOSFS_Block_Read(dev, array[newIndex/INT_ARR_SIZE], array2);
            Clear_Bit(bitmap, array2[newIndex % INT_ARR_SIZE]);
            Clear_Bit(bitmap, array[newIndex/INT_ARR_SIZE]);
            Clear_Bit(bitmap, node->blocks[DBL_BLOCK]);

        }
    }

    Free(array);
    Free(array2);
}



/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */

/*
 * Format a drive with GOSFS.
 */
int GOSFS_Format(struct Block_Device *blockDev) {
    uint_t numDiskBlocks = blockDev->ops->Get_Num_Blocks(blockDev); 
    uint_t numGOSFSBlocks = numDiskBlocks/DISK_BLOCKS_PER_GOSFS; 

    /* Allocate 4KB for super block */
    GOSFSsuperblock *super = Safe_Calloc(PAGE_SIZE);

    /* Creating bitmap and memcpying to super is redudant, so I skipped it */ 

    /* Create root directory */
    GOSFSdirectory *rootDir = Safe_Calloc(PAGE_SIZE);

    /* Initialize values in the super block */
    super->rootDir = 1; // Rootdir will be at GOSFS block[1]
    super->size = numGOSFSBlocks;
    Set_Bit(&super->bitmap, 0); // GOSFS block[0] used for superblock
    Set_Bit(&super->bitmap, 1); // GOSFS block[1] used for root dir
    super->magic = GOSFS_MAGIC;

    /* Write superblock to disk */ 
    GOSFS_Block_Write(blockDev, 0, super);
        
    /* Write root dir to disk */
    GOSFS_Block_Write(blockDev, 1, rootDir);

    /* Free memory used */
    Free(super);
    Free(rootDir);
    
    return 0;
}

/*
 * Mount GOSFS. Return 0 on success, return < 0 on failure.
 * - Check that the magic number is correct.
 */
int GOSFS_Mount(struct Mount_Point *mountPoint) {
    GOSFSsuperblock *super = Safe_Calloc(PAGE_SIZE);

    /* Read in the superblock from disk */
    GOSFS_Block_Read(mountPoint->dev, 0, super);

    /* Check to make sure correct filesystem type, via magic number */
    if (super->magic != GOSFS_MAGIC) {
        return EINVALIDFS;
    }

    /* Set the mount point functions */
    mountPoint->ops = &s_gosfsMountPointOps;   

    /* Set the mount point fsData to a copy of the superblock in memory */
    mountPoint->fsData = super;

    return 0;
}

/*
 * Get metadata for given File. Called with a file descriptor.
 */
int GOSFS_FStat(struct File *file, struct VFS_File_Stat *stat) {
    GOSFSptr *entry = (GOSFSptr *)file->fsData;
    Copy_Stat(stat, &entry->node);
    return 0;
}

/*
 * Open a file with the given name and mode.
 * Return 0 on success, < 0 on failure (e.g. does not exist).
 */
int GOSFS_Open(struct Mount_Point *mountPoint, const char *path, int mode,
               struct File **pFile) {
    int rc = 0;
    int blockNum = 0; // The GOSFS block the fileNode(file or dir) exists in
    char name[MAX_NAME_SIZE] = {};
    struct File *file = 0;
    bool found = 0; 
    GOSFSsuperblock *super = (GOSFSsuperblock *)mountPoint->fsData;
    GOSFSptr *gosfsEntry = Safe_Calloc(sizeof(GOSFSptr));
    GOSFSdirectory *currDir = Safe_Calloc(PAGE_SIZE); 

    /* Grab the root directory from the disk */
    blockNum = super->rootDir;
    GOSFS_Block_Read(mountPoint->dev, super->rootDir, currDir);

    /* Iterate through the path until reaching the end */
    while (*path != 0) {
        memset(name, '\0', MAX_NAME_SIZE); // Reset name to empty
        memset(gosfsEntry, '\0', sizeof(GOSFSptr)); // Reset GOSFSentry to empty
        Get_Next_Name_In_Path(&path, name);
        found = GOSFS_Lookup(currDir, name, blockNum, gosfsEntry); 

       /* If entry does not exist and it's not the end of the path */
        if (found == false && *path != 0) {
            rc = ENOTFOUND;
            goto fail;
        } 

        /* Entry's file and not end of path */
        if (found == true && gosfsEntry->node.isDirectory == 0 && *path != 0) {
            rc = ENOTDIR;
            goto fail;
        }
   
        /* If entry exists but is a directory, search in that directory */
        if (found == true && gosfsEntry->node.isDirectory == 1 && *path != 0) {
            blockNum =  gosfsEntry->node.blocks[0];
            GOSFS_Block_Read(mountPoint->dev, blockNum, currDir);
            continue;
        }

    }  

    /* Fail if open called on directory since we have an openDirectory */
    if (found == true && gosfsEntry->node.isDirectory == 1) {
        rc = EUNSUPPORTED;
        goto fail;
    }

    /* If entry exist and is a file then open it up */
    if (found == true && gosfsEntry->node.isDirectory == 0) {
        file = Allocate_File(&s_gosfsFileOps, 0, 
                        gosfsEntry->node.size, gosfsEntry, mode, mountPoint);
        if (file == 0) {
            goto memfail;
        }
        *pFile = file;
        goto done;
    }
        
    /* If entry does not exist and not trying to create */
    if (found == false && (mode & O_CREATE) == 0) {
        rc = ENOTFOUND; 
        goto fail;
    }

    /* If entry does not exist but want to create  */ 
    if (found == false && (mode & O_CREATE) == 1) {
        if (Create_File(currDir, name, blockNum, gosfsEntry) < 0) {
            rc = ENOSPACE;
            goto fail;
        }
        file = Allocate_File(&s_gosfsFileOps, 0, 0, gosfsEntry, mode, mountPoint);
        if (file == 0) {
            goto memfail;
        }
        *pFile = file;
        
        /* Write currDir back to disk */
        GOSFS_Block_Write(mountPoint->dev, blockNum, currDir);

        goto done;
    } 


  memfail:
    rc = ENOMEM;
  fail:
    Free(gosfsEntry);
  done:
    Free(currDir);
    return rc;
}

/*
 * Read data from current position in file.
 * Return > 0 on success, < 0 on failure.
 */
int GOSFS_Read(struct File *file, void *buf, ulong_t numBytes) {
    int bytesRead = 0;
    int bytesToRead = 0;
    int blockNum = 0;
    int offset = file->filePos % PAGE_SIZE;
    int endPos = file->filePos + numBytes;
    int numDirEntries = 0;
    int dirEntriesRead = 0;
    int index = 0;
    void *tempBuffer = 0;
    struct Block_Device *dev = file->mountPoint->dev;
    GOSFSptr *entry = (GOSFSptr *)file->fsData;
    GOSFSfileNode *node = &entry->node;
    GOSFSfileNode *currNode = 0;
    GOSFSdirectory *currDir = 0;
    struct VFS_Dir_Entry *dirEntry = 0;

    /* If the read is called on a directory, read dir entries into buffer */
    if (node->isDirectory == 1) {
        currDir = Safe_Calloc(PAGE_SIZE);
        dirEntry = Safe_Calloc(sizeof(struct VFS_Dir_Entry));
        numDirEntries = numBytes; // Num dir entries I need to read

        if ((file->filePos + numDirEntries) > file->endPos) {
            numDirEntries = file->endPos - file->filePos;
        }

        GOSFS_Block_Read(dev, node->blocks[0], currDir);

        /* Write the number of dir entries we need to the buffer */
        while (numDirEntries > 0) {
            memset(dirEntry, '\0', sizeof(struct VFS_Dir_Entry));
            currNode = &currDir->files[file->filePos];

            memcpy(dirEntry->name, currNode->name, MAX_NAME_SIZE);
            Copy_Stat(&dirEntry->stats, currNode);
            
            memcpy(buf, dirEntry, sizeof(struct VFS_Dir_Entry));
            
            buf += sizeof(struct VFS_Dir_Entry);
            numDirEntries--;
            dirEntriesRead++;
            file->filePos++;
        }

        Free(dirEntry);
        Free(currDir);
        return dirEntriesRead;
    } 
    /* If read is called on a file */
    else {

        /* No read access */
        if ((file->mode & O_READ) == 0) {
            return EACCESS; 
        }

        /* If trying to read more bytes than exists, only read what's left */
        if (endPos > file->endPos) {
            numBytes = file->endPos - file->filePos;
        }

        tempBuffer = Safe_Calloc(PAGE_SIZE);

        while (numBytes > 0) {
            memset(tempBuffer, '\0', PAGE_SIZE);
            blockNum = Get_Blocknum_To_Read(file);
            if (blockNum == 0) 
                return -1;

            if (numBytes < (PAGE_SIZE - offset)) {
                bytesToRead = numBytes;
            } else if (numBytes >= (PAGE_SIZE - offset)) {
                bytesToRead = PAGE_SIZE - offset; 
            }

            GOSFS_Block_Read(dev, blockNum, tempBuffer);
            memcpy(buf, tempBuffer + offset, bytesToRead);
            offset = 0;

            file->filePos += bytesToRead; 
            bytesRead += bytesToRead;
            numBytes -= bytesToRead;
        }
        Free(tempBuffer);
        return bytesRead; 
    }
}

/*
 * Write data to current position in file.
 * Return > 0 on success, < 0 on failure.
 */
int GOSFS_Write(struct File *file, void *buf, ulong_t numBytes) {
    int bytesCopied = 0;
    int blockNum = 0;
    int bytesToCopy = 0;
    int sizeIncrease = 0;
    int offset = file->filePos % PAGE_SIZE;
    void *tempBuffer = Safe_Calloc(PAGE_SIZE);
    struct Block_Device *dev = file->mountPoint->dev; 
    GOSFSsuperblock *super = (GOSFSsuperblock *)file->mountPoint->fsData; //superblock info
    GOSFSptr *entry = (GOSFSptr *)file->fsData; 
    int origBlock = entry->blockNum;
    GOSFSdirectory *currDir = Safe_Calloc(PAGE_SIZE);
    
    /* No write access */
    if ((file->mode & O_WRITE) == 0) {
        return EACCESS;
    } 

    /* Can't write to a directory */
    if (entry->node.isDirectory == 1) {
        return EUNSUPPORTED;
    }

    /* Calculate how much the file size will increase */
    sizeIncrease = (file->endPos - file->filePos) + numBytes;

    /* Grab the current directory so I can emulate changes to the file on the disk */
    GOSFS_Block_Read(dev, origBlock, currDir);

    /* Write whole buffer to disk */
    while (numBytes > 0) {
        memset(tempBuffer, '\0', PAGE_SIZE);
        blockNum = Get_Blocknum_To_Write(file);

        if (numBytes < (PAGE_SIZE - offset)) {
            bytesToCopy = numBytes;
        } else if (numBytes >= (PAGE_SIZE - offset)) {
            bytesToCopy = PAGE_SIZE - offset;
        } 

        GOSFS_Block_Read(dev, blockNum, tempBuffer); 
        memcpy(tempBuffer + offset, buf, bytesToCopy);
        GOSFS_Block_Write(dev, blockNum, tempBuffer);   
        offset = 0; // no more offset needed
        
        file->filePos += bytesToCopy;
        bytesCopied += bytesToCopy;
        numBytes -= bytesToCopy;
    }

    /* Copy changes to the fileNode back to currDir and write back to disk */
    entry->node.size += sizeIncrease;
    file->endPos += sizeIncrease;
    memcpy(&currDir->files[entry->offset], &entry->node, sizeof(GOSFSfileNode));
    GOSFS_Block_Write(dev, origBlock, currDir);

    /* Copy changes in superblock back to disk */
    GOSFS_Block_Write(dev, 0, super);
    
  fail:
  done:
    Free(tempBuffer);
    Free(currDir);
    return bytesCopied;
}

/*
 * Get metadata for given file. Need to find the file from the given path.
 */
int GOSFS_Stat(struct Mount_Point *mountPoint, const char *path,
               struct VFS_File_Stat *stat) {
    int rc = 0;
    int blockNum = 0; // The GOSFS block the fileNode(file or dir) exists in
    bool found = 0;
    char name[MAX_NAME_SIZE] = {};
    GOSFSsuperblock *super = (GOSFSsuperblock *)mountPoint->fsData;
    GOSFSptr *gosfsEntry = Safe_Calloc(sizeof(GOSFSptr));
    GOSFSdirectory *currDir = Safe_Calloc(PAGE_SIZE);

    /* Grab the root directory from disk */
    blockNum = super->rootDir;
    GOSFS_Block_Read(mountPoint->dev, super->rootDir, currDir);

    while(*path != 0) {
        memset(name, '\0', MAX_NAME_SIZE);
        memset(gosfsEntry, '\0', sizeof(GOSFSptr));
        Get_Next_Name_In_Path(&path, name);
        found = GOSFS_Lookup(currDir, name, blockNum, gosfsEntry);

        /* Entry exists but not a file and not end of path */
        if (found == true && gosfsEntry->node.isDirectory == 0 && *path != 0) {
            rc = ENOTDIR;
            goto fail;
        }
            
        /* Entry exists but directory ,search in that dir */
        if (found == true && gosfsEntry->node.isDirectory == 1 && *path != 0) {
            blockNum =  gosfsEntry->node.blocks[0];
            GOSFS_Block_Read(mountPoint->dev, blockNum, currDir);
            continue;
        }

        /* If entry does not exist then fail */
        if (found == false ) {
            rc = ENOTFOUND;
            goto fail;
        }

    }

    /* Copy metadata */
    Copy_Stat(stat, &gosfsEntry->node);

  fail:
  done:
    Free(gosfsEntry);
    Free(currDir);
    return rc;
}

/*
 * Synchronize the filesystem data with the disk
 * (i.e., flush out all buffered filesystem data).
 */
int GOSFS_Sync(struct Mount_Point *mountPoint) {
    return 0;
}

/*
 * Close a file. 
 */
int GOSFS_Close(struct File *file) {
    Free((GOSFSptr *)file->fsData);
    return 0;
}

/*
 * Create a directory named by given path.
 */
int GOSFS_Create_Directory(struct Mount_Point *mountPoint, const char *path) {
    int rc = 0;
    int blockNum = 0; // The GOSFS block the fileNode(file or dir) exists in
    int newBlock = 0;
    int index = 0;
    char name[MAX_NAME_SIZE] = {};
    bool found = 0;
    GOSFSfileNode *currNode = 0;
    GOSFSsuperblock *super = (GOSFSsuperblock *)mountPoint->fsData;
    GOSFSptr *gosfsEntry = Safe_Calloc(sizeof(GOSFSptr));
    GOSFSdirectory *currDir = Safe_Calloc(PAGE_SIZE);

    /* Grab the root directory from the disk */
    GOSFS_Block_Read(mountPoint->dev, super->rootDir, currDir);
    blockNum = super->rootDir;

    /* Create all of the directories in the path if they don't exist */
    while (*path != 0) {
        memset(name, '\0', MAX_NAME_SIZE);
        memset(gosfsEntry, '\0', sizeof(GOSFSptr));
        newBlock = 0;
        currNode = 0;
        Get_Next_Name_In_Path(&path, name);
        found = GOSFS_Lookup(currDir, name, blockNum, gosfsEntry);

        /* If last element in path exists then fail */
        if (found == true && *path == 0) {
            rc = EEXIST;
            goto fail;
        }

        /* If anything in path is a file then fail */
        if (found == true && gosfsEntry->node.isDirectory == 0) {
            rc = ENOTDIR;
            goto fail;
        }
        
        /* If entry is a directory, then go into it */
        if (found == true && gosfsEntry->node.isDirectory == 1) {
            blockNum = gosfsEntry->node.blocks[0];
            GOSFS_Block_Read(mountPoint->dev, blockNum, currDir);
            continue;
        }

        /* If entry does not exist, then make it */
        if (found == false) {
            /* Find the next open spot in the directory */
            for (index = 0; index < MAX_FILES_PER_DIR; index++) {
                currNode = &currDir->files[index];
                if (currNode->isUsed == 0) {
                    /* Allocate new GOSFS block for new dir */
                    newBlock = Find_First_Free_Bit(&super->bitmap, super->size);
                    Set_Bit(&super->bitmap, newBlock);

                    /* Set up new dir */
                    memcpy(currNode->name, name, MAX_NAME_SIZE);  
                    currNode->size = MAX_FILES_PER_DIR;
                    currNode->isUsed = 1;
                    currNode->isDirectory = 1;
                    currNode->blocks[0] = newBlock;

                    /* Write changes to the curr dir back to disk */
                    GOSFS_Block_Write(mountPoint->dev, blockNum, currDir);

                    memset(currDir, '\0', PAGE_SIZE); // new curr dir empty
                    
                    GOSFS_Block_Write(mountPoint->dev, newBlock, currDir);
                    blockNum = newBlock;
                    
                    break;
                }
                /* If no more room in curr dir */
                if ((index + 1) == MAX_FILES_PER_DIR) {
                    rc = ENOMEM;
                    goto fail;
                }
            }
                      
        }

    }

    /* Write super block back to disk */
    GOSFS_Block_Write(mountPoint->dev, 0, super);

  fail:
  done:
    Free(currDir);
    Free(gosfsEntry);
    return rc;
  
}

/*
 * Open a directory named by given path.
 */
int GOSFS_Open_Directory(struct Mount_Point *mountPoint, const char *path,
                         struct File **pDir) {
    int rc = 0;
    int blockNum = 0; // The GOSFS block the fileNode(file or dir) exists in
    char name[MAX_NAME_SIZE] = {};
    struct File *dir = 0;
    bool found = 0; 
    GOSFSsuperblock *super = (GOSFSsuperblock *)mountPoint->fsData;
    GOSFSptr *gosfsEntry = Safe_Calloc(sizeof(GOSFSptr));
    GOSFSdirectory *currDir = Safe_Calloc(PAGE_SIZE); 

    /* Grab the root directory from the disk */
    blockNum = super->rootDir;
    GOSFS_Block_Read(mountPoint->dev, blockNum, currDir);

    /* Iterate through the path until reaching the end */
    while (*path != 0) {
        memset(name, '\0', MAX_NAME_SIZE); // Reset name to empty
        memset(gosfsEntry, '\0', sizeof(GOSFSptr)); // Reset GOSFSentry to empty
        Get_Next_Name_In_Path(&path, name);
        found = GOSFS_Lookup(currDir, name, blockNum, gosfsEntry); 

       /* If dir does not exist */
        if (found == false) {
            rc = ENOTFOUND;
            goto fail;
        } 

        /* If entry exists but is a file */
        if (found == true && gosfsEntry->node.isDirectory == 0) {
            rc = ENOTDIR;
            goto fail;
        }
        
        /* If dir exists search in that directory */
        if (found == true && gosfsEntry->node.isDirectory == 1 && *path != 0) {
            blockNum =  gosfsEntry->node.blocks[0];
            GOSFS_Block_Read(mountPoint->dev, blockNum, currDir);
            continue;
        }

    }  

    /* Open directory  */
    dir = Allocate_File(&s_gosfsDirOps, 0, MAX_FILES_PER_DIR, 
                        gosfsEntry, 0, mountPoint); 
    if (dir ==0) {
        goto memfail;
    }
    *pDir = dir;
    goto done;

  memfail:
    rc = ENOMEM;
  fail:
    Free(gosfsEntry);
  done:
    Free(currDir);
    return rc;
}

/*
 * Seek to a position in file. Should not seek beyond the end of the file.
 */
int GOSFS_Seek(struct File *file, ulong_t pos) {
    if (pos > file->endPos || pos < 0) {
        return EUNSPECIFIED;
    }
    file->filePos = pos;    
    return 0;
}

/*
 * Delete the given file.
 * Return > 0 on success, < 0 on failure.
 */
int GOSFS_Delete(struct Mount_Point *mountPoint, const char *path) {
    int rc = 0;
    int blockNum = 0; // The GOSFS block the fileNode(file or dir) exists in
    int index = 0;
    char name[MAX_NAME_SIZE] = {};
    struct File *dir = 0;
    bool found = 0; 
    GOSFSsuperblock *super = (GOSFSsuperblock *)mountPoint->fsData;
    GOSFSptr *gosfsEntry = Safe_Calloc(sizeof(GOSFSptr));
    GOSFSdirectory *currDir = Safe_Calloc(PAGE_SIZE); 
    GOSFSdirectory *tempDir = Safe_Calloc(PAGE_SIZE);
    GOSFSfileNode *currNode = 0;

    /* Grab root dir from disk */
    blockNum = super->rootDir;
    GOSFS_Block_Read(mountPoint->dev, blockNum, currDir);

    /* Find file node */
    while (*path != 0) {
        memset(name, '\0', MAX_NAME_SIZE);
        memset(gosfsEntry, '\0', sizeof(GOSFSptr));
        Get_Next_Name_In_Path(&path, name);
        found = GOSFS_Lookup(currDir, name, blockNum, gosfsEntry);

        /* Entry exists but not a file and not end of path */
        if (found == true && gosfsEntry->node.isDirectory == 0 && *path != 0) {
            rc = ENOTDIR;
            goto fail;
        }

        /* Entry exists but directory ,search in that dir */
        if (found == true && gosfsEntry->node.isDirectory == 1 && *path != 0) {
            blockNum =  gosfsEntry->node.blocks[0];
            GOSFS_Block_Read(mountPoint->dev, blockNum, currDir);
            continue;
        }

        if (found == false) {
            rc = ENOTFOUND;
            goto fail;
        }
    }

    /* If directory */
    if (gosfsEntry->node.isDirectory == 1) {
        /* Check if directory is empty and fail if it isn't */
        GOSFS_Block_Read(mountPoint->dev, gosfsEntry->node.blocks[0], tempDir);
        for (index = 0; index < MAX_FILES_PER_DIR; index++) {
            currNode = &tempDir->files[index];
            if (currNode->isUsed != 0) {
                rc = EBUSY;
                goto fail;
            }
        }
        Clear_Bit(&super->bitmap, gosfsEntry->node.blocks[0]);  
        memset(&currDir->files[gosfsEntry->offset], '\0', sizeof(GOSFSfileNode));
    } 
    /* Else file */
    else {
        Clear_File_Bits(&super->bitmap, mountPoint->dev, gosfsEntry);
        memset(&currDir->files[gosfsEntry->offset], '\0', sizeof(GOSFSfileNode));
    }

    /* Copy changes back to disk */
    GOSFS_Block_Write(mountPoint->dev, blockNum, currDir);
    GOSFS_Block_Write(mountPoint->dev, 0, super);


  fail:
  done:
    Free(tempDir);
    Free(gosfsEntry);
    Free(currDir);
    return rc;
}

/*
 * Read next directory entry from an open directory.
 * Return 1 on success, < 0 on failure.
 */
int GOSFS_Read_Entry(struct File *dir, struct VFS_Dir_Entry *entry) {
    int rc = 1;
    GOSFSfileNode *currNode = 0;
    GOSFSdirectory *currDir = Safe_Calloc(PAGE_SIZE);
    GOSFSptr *currEntry = (GOSFSptr *)dir->fsData;

    /* struct File is not a directory */
    if (currEntry->node.isDirectory == 0) {
        rc = ENOTDIR;
        goto fail;
    }

    /* file position at end of dir */
    if (dir->filePos >= MAX_FILES_PER_DIR) {
        rc = EINVALID;
        goto fail;
    }

    /* Grab the open directory's actual directory structure from disk */
    GOSFS_Block_Read(dir->mountPoint->dev, currEntry->node.blocks[0], currDir);

    /* Get the next non-empty entry */
    for (; dir->filePos < MAX_FILES_PER_DIR; dir->filePos++) {
        currNode = &currDir->files[dir->filePos]; 
        if (currNode->isUsed == 1) {
            memcpy(entry->name, currNode->name, MAX_NAME_SIZE);
            Copy_Stat(&entry->stats, currNode);
            dir->filePos++;
            goto done;
        }
    }

    rc = ENOTFOUND;

  fail:
  done:
    Free(currDir);
    return rc;

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
