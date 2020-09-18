#ifndef __FS_H__
#define __FS_H__

#include "fs/minix.h"

int readSuperBlock (SuperBlock *superBlock);

int allocInode (SuperBlock *superBlock,
                Inode *fatherInode,
                int fatherInodeOffset,
                Inode *destInode,
                int *destInodeOffset,
                const char *destFilename,
                int destFiletype);

int freeInode (SuperBlock *superBlock,
               Inode *fatherInode,
               int fatherInodeOffset,
               Inode *destInode,
               int *destInodeOffset,
               const char *destFilename,
               int destFiletype);

int readInode (SuperBlock *superBlock,
               Inode *destInode,
               int *inodeOffset,
               const char *destFilePath);

int allocBlock (SuperBlock *superBlock,
                Inode *inode,
                int inodeOffset);

int readBlock (SuperBlock *superBlock,
               Inode *inode,
               int blockIndex,
               uint8_t *buffer);

int writeBlock (SuperBlock *superBlock,
                Inode *inode,
                int blockIndex,
                uint8_t *buffer);

int getDirEntry (SuperBlock *superBlock,
                 Inode *inode,
                 int dirIndex,
                 DirEntry *destDirEntry);

void initFS (void);

void initFile (void);

#endif /* __FS_H__ */
