#include <stdio.h>
#include "utils.h"
#include "data.h"
#include "func.h"

/*
 *  When sectorNum < 64, FS is too small, return -1.
 *  Initialize superblock and write back to first block in FS.
 *  All offset is sector as unit, index of sector.
 */
int initSuperBlock (FILE *file, int sectorNum, int sectorsPerBlock, SuperBlock *superBlock) {
    if (sectorNum < 64)
        return -1;
    int inodeBitmapOffset = sizeof(SuperBlock) / SECTOR_SIZE;
    int blockBitmapOffset = inodeBitmapOffset + INODE_BITMAP_SIZE / SECTOR_SIZE;
    int inodeTableOffset = blockBitmapOffset + BLOCK_BITMAP_SIZE / SECTOR_SIZE;
    int blocksOffset = inodeTableOffset + INODE_BLOCKS * sectorsPerBlock;
    int inodeNum = INODE_BLOCKS * SECTOR_SIZE * sectorsPerBlock / sizeof(Inode);
    int blockNum = sectorNum / sectorsPerBlock - 3 - INODE_BLOCKS;
    superBlock->sectorNum = sectorNum;
    superBlock->inodeNum = (inodeNum / 8) * 8;
    superBlock->blockNum = (blockNum / 8) * 8;
    superBlock->availInodeNum = (inodeNum / 8) * 8;
    superBlock->availBlockNum = (blockNum / 8) * 8;
    superBlock->blockSize = SECTOR_SIZE * sectorsPerBlock;
    superBlock->inodeBitmap = inodeBitmapOffset;
    superBlock->blockBitmap = blockBitmapOffset;
    superBlock->inodeTable = inodeTableOffset;
    superBlock->blocks = blocksOffset;
    fseek(file, 0, SEEK_SET);
    fwrite((void *)superBlock, sizeof(SuperBlock), 1, file);
    return 0;
}

/*
 *  Read superblock to superBlock
 */
int readSuperBlock (FILE *file, SuperBlock *superBlock) {
    fseek(file, 0, SEEK_SET);
    fread((void*)superBlock, sizeof(SuperBlock), 1, file);
    if (superBlock->sectorNum < 64)
        return -1;
    return 0;
}

/*
 *  Read the blockIndex-th block of inode to buffer.
 *  Return -1 when failed.
 */
int readBlock (FILE *file, SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer) {
    // calculate the index and bound
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];
    
    if (blockIndex < bound0) {
        fseek(file, inode->pointer[blockIndex] * SECTOR_SIZE, SEEK_SET);
        fread((void *)buffer, sizeof(uint8_t), superBlock->blockSize, file);
        return 0;
    }
    else if (blockIndex < bound1) {
        fseek(file, inode->singlyPointer * SECTOR_SIZE, SEEK_SET);
        fread((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, file);
        fseek(file, singlyPointerBuffer[blockIndex - bound0] * SECTOR_SIZE, SEEK_SET);
        fread((void *)buffer, sizeof(uint8_t), superBlock->blockSize, file);
        return 0;
    }
    else
        return -1;
}

/*
 *  Find the Inode of destFilePath, '/' to first inode, '/bin' to inode in 'bin'
 *  Input: file (disk), superBlock
 *  OutPut: destInode, inodeOffset (byte as unit)
 *  Return -1 when failed.
 */
int readInode (FILE *file, SuperBlock *superBlock, Inode *destInode,
        int *inodeOffset, const char *destFilePath) {
    int i = 0;
    int j = 0;
    int ret = 0;
    int cond = 0;
    *inodeOffset = 0;
    uint8_t buffer[superBlock->blockSize];
    DirEntry *dirEntry = NULL;
    int count = 0;
    int size = 0;
    int blockCount = 0;

    // no content in destFilePath
    if (destFilePath == NULL || destFilePath[count] == 0)
        return -1;

    // destFilePath is not started with '/'
    ret = stringChr(destFilePath, '/', &size);
    if (ret == -1 || size != 0)
        return -1;

    // load the first Inode
    *inodeOffset = superBlock->inodeTable * SECTOR_SIZE;
    fseek(file, *inodeOffset, SEEK_SET);
    fread((void *)destInode, sizeof(Inode), 1, file);

    count += 1;
    while(destFilePath[count] != 0) {
        ret = stringChr(destFilePath + count, '/', &size);
        // pattern '//' occured
        if (ret == 0 && size == 0)
            return -1;
        // no more '/'
        if (ret == -1)
            cond = 1;
        // with more '/' but regular file
        else if (destInode->type == REGULAR_TYPE)
            return -1;
        
        // go deeper dir
        blockCount = destInode->blockCount;
        for (i = 0; i < blockCount; i++) {
            ret = readBlock(file, superBlock, destInode, i, buffer);
            if (ret == -1)
                return -1;
            // try find matched DirEntry and Inode
            dirEntry = (DirEntry *)buffer;
            for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j++) {
                if (dirEntry[j].inode == 0)
                    continue;
                else if (stringCmp(dirEntry[j].name, destFilePath + count, size) == 0) {
                    *inodeOffset = superBlock->inodeTable * SECTOR_SIZE + (dirEntry[j].inode - 1) * sizeof(Inode);
                    fseek(file, *inodeOffset, SEEK_SET);
                    fread((void *)destInode, sizeof(Inode), 1, file);
                    break;
                }
            }
            if (j < superBlock->blockSize / sizeof(DirEntry))
                break;
        }
        if (i < blockCount) {
            if (cond == 0)
                count += (size + 1);
            else
                return 0;
        }
        else
            return -1;
    }
    return 0;
}

/*
 * return the number of pointerBlock should be allocated in addition if the blockCount-th block is added
 */
int calNeededPointerBlocks (SuperBlock *superBlock, int blockCount) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    if (blockCount == bound0)
        return 1;
    else if (blockCount >= bound1)
        return -1;
    else
        return 0;
}

/*
 *  Find a Block unused, this function will update SuperBlock and BlockBitmap and write back.
 *  Input: file, superblock.
 *  Output: blockOffset (sector as unit)
 *  Return -1 when failed.
 */
int getAvailBlock (FILE *file, SuperBlock *superBlock, int *blockOffset) {
    int j = 0;
    int k = 0;
    int blockBitmapOffset = 0;
    BlockBitmap blockBitmap;

    if (superBlock->availBlockNum == 0)
        return -1;
    superBlock->availBlockNum--;

    blockBitmapOffset = superBlock->blockBitmap;
    fseek(file, blockBitmapOffset * SECTOR_SIZE, SEEK_SET);
    fread((void *)&blockBitmap, sizeof(BlockBitmap), 1, file);
    for (j = 0; j < superBlock->blockNum / 8; j++) {
        if (blockBitmap.byte[j] != 0xff) {
            break;
        }
    }
    for (k = 0; k < 8; k++) {
        if ((blockBitmap.byte[j] >> (7 - k)) % 2 == 0) {
            break;
        }
    }
    blockBitmap.byte[j] = blockBitmap.byte[j] | (1 << (7 - k));
    
    *blockOffset = superBlock->blocks + ((j * 8 + k) * superBlock->blockSize / SECTOR_SIZE);

    fseek(file, 0, SEEK_SET);
    fwrite((void *)superBlock, sizeof(SuperBlock), 1, file);
    fseek(file, blockBitmapOffset * SECTOR_SIZE, SEEK_SET);
    fwrite((void *)&blockBitmap, sizeof(BlockBitmap), 1, file);

    return 0;
}

/*
 *  Alloc a new Block for inode, inode.blockCount++ and write back inode.
 *  Input: file, superBlock, inode, inodeOffset(byte as unit), blockOffset (sector as unit).
 *  Output: inode
 *  Return -1 when failed.
 */
int allocLastBlock (FILE *file, SuperBlock *superBlock, Inode *inode, int inodeOffset, int blockOffset) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];
    int singlyPointerBufferOffset = 0;

    if (inode->blockCount < bound0) {
        inode->pointer[inode->blockCount] = blockOffset;
    }
    else if (inode->blockCount == bound0) {
        getAvailBlock(file, superBlock, &singlyPointerBufferOffset);
        singlyPointerBuffer[0] = blockOffset;
        inode->singlyPointer = singlyPointerBufferOffset;
        fseek(file, singlyPointerBufferOffset * SECTOR_SIZE, SEEK_SET);
        fwrite((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, file);
    }
    else if (inode->blockCount < bound1) {
        fseek(file, inode->singlyPointer * SECTOR_SIZE, SEEK_SET);
        fread((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, file);
        singlyPointerBuffer[inode->blockCount - bound0] = blockOffset;
        fseek(file, inode->singlyPointer * SECTOR_SIZE, SEEK_SET);
        fwrite((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, file);
    }
    else
        return -1;
    
    inode->blockCount++;
    fseek(file, inodeOffset, SEEK_SET);
    fwrite((void *)inode, sizeof(Inode), 1, file);
    return 0;
}

/*
 *  Alloc a new block for inode by call getAvailBlock and allocLastBlock.
 *  Input: file, superBlock, inode, inodeOffset (byte as unit).
 *  Output: inode.
 *  Return -1 when failed.
 */
int allocBlock (FILE *file, SuperBlock *superBlock, Inode *inode, int inodeOffset) {
    int ret = 0;
    int blockOffset = 0;

    ret = calNeededPointerBlocks(superBlock, inode->blockCount);
    if (ret == -1)
        return -1;
    if (superBlock->availBlockNum < ret + 1)
        return -1;
    
    getAvailBlock(file, superBlock, &blockOffset);
    allocLastBlock(file, superBlock, inode, inodeOffset, blockOffset);
    return 0;
}

/*
 *  Find a Inode unused, update superBlock InodeBitmap and write back.
 *  Input: file, superblock.
 *  Output: inodeOffset (byte as unit).
 *  Return -1 when failed.
 */
int getAvailInode (FILE *file, SuperBlock *superBlock, int *inodeOffset) {
    int j = 0;
    int k = 0;
    int inodeBitmapOffset = 0;
    int inodeTableOffset = 0;
    InodeBitmap inodeBitmap;

    if (superBlock->availInodeNum == 0)
        return -1;
    superBlock->availInodeNum--;

    inodeBitmapOffset = superBlock->inodeBitmap;
    inodeTableOffset = superBlock->inodeTable;
    fseek(file, inodeBitmapOffset * SECTOR_SIZE, SEEK_SET);
    fread((void *)&inodeBitmap, sizeof(InodeBitmap), 1, file);
    for (j = 0; j < superBlock->availInodeNum / 8; j++) {
        if (inodeBitmap.byte[j] != 0xff) {
            break;
        }
    }
    for (k = 0; k < 8; k++) {
        if ((inodeBitmap.byte[j] >> (7-k)) % 2 == 0) {
            break;
        }
    }
    inodeBitmap.byte[j] = inodeBitmap.byte[j] | (1 << (7 - k));

    *inodeOffset = inodeTableOffset * SECTOR_SIZE + (j * 8 + k) * sizeof(Inode);

    fseek(file, 0, SEEK_SET);
    fwrite((void *)superBlock, sizeof(SuperBlock), 1, file);
    fseek(file, inodeBitmapOffset * SECTOR_SIZE, SEEK_SET);
    fwrite((void *)&inodeBitmap, sizeof(InodeBitmap), 1, file);

    return 0;
}

/*
 *  Write buffer to the blockIndex-th block of inode.
 *  Return -1 when failed.
 */
int writeBlock (FILE *file, SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];

    if (blockIndex < bound0) {
        fseek(file, inode->pointer[blockIndex] * SECTOR_SIZE, SEEK_SET);
        fwrite((void *)buffer, sizeof(uint8_t), superBlock->blockSize, file);
        return 0;
    }
    else if (blockIndex < bound1) {
        fseek(file, inode->singlyPointer * SECTOR_SIZE, SEEK_SET);
        fread((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, file);
        fseek(file, singlyPointerBuffer[blockIndex - bound0] * SECTOR_SIZE, SEEK_SET);
        fwrite((void *)buffer, sizeof(uint8_t), superBlock->blockSize, file);
        return 0;
    }
    else
        return -1;
}

/*
 *  Read fatherInode blocks and find a empty DirEntry.
 *  Find a empty Inode and fill DirEntry.name and DirEntry.inode.
 *  New DirEntry should contrain '.' and '..'.
 */
int allocInode (FILE *file, SuperBlock *superBlock, Inode *fatherInode, int fatherInodeOffset,
        Inode *destInode, int *destInodeOffset, const char *destFilename, int destFiletype) {
    int i = 0;
    int j = 0;
    int ret = 0;
    int blockOffset = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];
    int length = stringLen(destFilename);

    if (destFilename == NULL || destFilename[0] == 0)
        return -1;

    if (superBlock->availInodeNum == 0)
        return -1;
    
    for (i = 0; i < fatherInode->blockCount; i++) {
        ret = readBlock(file, superBlock, fatherInode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j++) {
            if (dirEntry[j].inode == 0) // a valid empty dirEntry
                break;
            else if (stringCmp(dirEntry[j].name, destFilename, length) == 0)
                return -1; // file with filename = destFilename exist
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == fatherInode->blockCount) {
        ret = allocBlock(file, superBlock, fatherInode, fatherInodeOffset);
        if (ret == -1)
            return -1;
        fatherInode->size = fatherInode->blockCount * superBlock->blockSize;
        setBuffer(buffer, superBlock->blockSize, 0);
        dirEntry = (DirEntry *)buffer;
        j = 0;
    }
    // dirEntry[j] is the valid empty dirEntry, it is in the i-th block of fatherInode.
    ret = getAvailInode(file, superBlock, destInodeOffset);
    if (ret == -1)
        return -1;

    stringCpy(destFilename, dirEntry[j].name, NAME_LENGTH);
    dirEntry[j].inode = (*destInodeOffset - superBlock->inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
    ret = writeBlock(file, superBlock, fatherInode, i, buffer);
    if (ret == -1)
        return -1;
    
    fseek(file, fatherInodeOffset, SEEK_SET);
    fwrite((void *)fatherInode, sizeof(Inode), 1, file);

    destInode->type = destFiletype;
    destInode->linkCount = 1;
    destInode->blockCount = 0;
    destInode->size = 0;

    fseek(file, *destInodeOffset, SEEK_SET);
    fwrite((void *)destInode, sizeof(Inode), 1, file);

    return 0;
}

int initDir(FILE *file, SuperBlock *superBlock, Inode *fatherInode, int fatherInodeOffset,
        Inode *destInode, int destInodeOffset) {
    int ret = 0;
    int blockOffset = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];

    ret = getAvailBlock(file, superBlock, &blockOffset);
    if (ret == -1)
        return -1;
    destInode->pointer[0] = blockOffset;
    destInode->blockCount = 1;
    destInode->size = superBlock->blockSize;
    setBuffer(buffer, superBlock->blockSize, 0);
    dirEntry = (DirEntry *)buffer;
    dirEntry[0].inode = (destInodeOffset - superBlock->inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
    destInode->linkCount ++;
    dirEntry[0].name[0] = '.';
    dirEntry[0].name[1] = '\0';
    dirEntry[1].inode = (fatherInodeOffset - superBlock->inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
    fatherInode->linkCount ++;
    dirEntry[1].name[0] = '.';
    dirEntry[1].name[1] = '.';
    dirEntry[1].name[2] = '\0';

    fseek(file, blockOffset * SECTOR_SIZE, SEEK_SET);
    fwrite((void *)buffer, sizeof(uint8_t), superBlock->blockSize, file);
    fseek(file, fatherInodeOffset, SEEK_SET);
    fwrite((void *)fatherInode, sizeof(Inode), 1, file);
    fseek(file, destInodeOffset, SEEK_SET);
    fwrite((void *)destInode, sizeof(Inode), 1, file);
    return 0;
}

int copyData (FILE *file, FILE *fileSrc, SuperBlock *superBlock, Inode *inode, int inodeOffset) {
    int i = 0;
    int ret = 0;
    int size = 0;
    int totalSize = 0;
    uint8_t buffer[superBlock->blockSize];
    fseek(fileSrc, 0, SEEK_SET);
    size = fread((void*)buffer, sizeof(uint8_t), superBlock->blockSize, fileSrc);

    while (size != 0) {
        if (i == inode->blockCount) {
            ret = allocBlock(file, superBlock, inode, inodeOffset);
            if (ret == -1)
                return -1;
        }
        ret = writeBlock(file, superBlock, inode, i, buffer);
        if (ret == -1)
            return -1;
        totalSize += size;
        i++;
        size = fread((void*)buffer, sizeof(uint8_t), superBlock->blockSize, fileSrc);
    }
    inode->blockCount = i;
    inode->size = totalSize;
    fseek(file, inodeOffset, SEEK_SET);
    fwrite((void*)inode, sizeof(Inode), 1, file);
    return 0;
}

int getDirEntry (FILE *file, SuperBlock *superBlock, Inode *inode, int dirIndex, DirEntry *destDirEntry) {
    int i = 0;
    int j = 0;
    int ret = 0;
    int dirCount = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];

    for (i = 0; i < inode->blockCount; i++) {
        ret = readBlock(file, superBlock, inode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j ++) {
            if (dirEntry[j].inode != 0) {
                if (dirCount == dirIndex)
                    break;
                else
                    dirCount ++;
            }
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == inode->blockCount)
        return -1;
    else {
        destDirEntry->inode = dirEntry[j].inode;
        stringCpy(dirEntry[j].name, destDirEntry->name, NAME_LENGTH);
        return 0;
    }
}
/*
 *  Root dir contain 2 dirEntry (. and ..).
 *  1 Inode and 1 block used.
 */
int initRootDir (FILE *file, SuperBlock *superBlock) {
    int inodeBitmapOffset = 0;
    int inodeTableOffset = 0;
    int inodeOffset = 0;    // XXX byte as unit
    int blockOffset = 0;    // XXX sector as unit
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];
    InodeBitmap inodeBitmap;
    Inode inode;

    if (superBlock->availInodeNum == 0)
        return -1;
    superBlock->availInodeNum --;

    inodeBitmapOffset = superBlock->inodeBitmap;    // XXX sector as unit
    inodeTableOffset = superBlock->inodeTable;      // XXX sector as unit
    inodeBitmap.byte[0] = 0x80;
    inodeOffset = inodeTableOffset * SECTOR_SIZE;

    inode.type = DIRECTORY_TYPE;
    getAvailBlock(file, superBlock, &blockOffset);
    inode.pointer[0] = blockOffset;
    inode.blockCount = 1;
    setBuffer(buffer, superBlock->blockSize, 0);
    dirEntry = (DirEntry *)buffer;
    dirEntry[0].inode = 1;
    dirEntry[0].name[0] = '.';
    dirEntry[0].name[1] = '\0';
    dirEntry[1].inode = 1;
    dirEntry[1].name[0] = '.';
    dirEntry[1].name[1] = '.';
    dirEntry[1].name[2] = '\0';
    inode.linkCount = 2;
    inode.size = superBlock->blockSize;

    fseek(file, blockOffset * SECTOR_SIZE, SEEK_SET);
    fwrite((void *)buffer, sizeof(uint8_t), superBlock->blockSize, file);

    fseek(file, inodeBitmapOffset * SECTOR_SIZE, SEEK_SET);
    fwrite((void *)&inodeBitmap, sizeof(uint8_t), 1, file);

    fseek(file, inodeOffset, SEEK_SET);
    fwrite((void *)&inode, sizeof(Inode), 1, file);
    
    fseek(file, 0, SEEK_SET);
    fwrite((void *)superBlock, sizeof(SuperBlock), 1, file);

    return 0;
}

/*
 *  Create a file as FS, initialize SuperBlock and Root dir.
 */
int format (const char *driver, int sectorNum, int sectorsPerBlock) {
    int i = 0;
    int ret = 0;
    FILE *file = NULL;
    uint8_t byte[SECTOR_SIZE];
    SuperBlock superBlock;
    if (driver == NULL) {
        printf("driver == NULL\n");
        return -1;
    }
    file = fopen(driver, "w+");
    if (file == NULL) {
        printf("Failed to open driver.\n");
        return -1;
    }
    for (i = 0; i < SECTOR_SIZE; i++) {
        byte[i] = 0;
    }
    for (i = 0; i < sectorNum; i++) {
        fwrite((void*)byte, sizeof(uint8_t), SECTOR_SIZE, file);
    }
    ret = initSuperBlock(file, sectorNum, sectorsPerBlock, &superBlock);
    if (ret == -1) {
        printf("Failed to format: No enough sectors.\n");
        fclose(file);
        return -1;
    }
    ret = initRootDir(file, &superBlock);
    if (ret == -1) {
        printf("Failed to fromat: No enough inodes or data blocks.\n");
        fclose(file);
        return -1;
    }
    printf("format %s -s %d -b %d\n", driver, sectorNum, sectorsPerBlock);
    printf("FORMAT success.\n%d inodes and %d data blocks available.\n", superBlock.availInodeNum, superBlock.availBlockNum);
    fclose(file);
    return 0;
}

/*
 *  'mkdir /mnt'.
 */
int mkdir (const char *driver, const char *destDirPath){
    FILE *file = NULL;
    char tmp = 0;
    int length = 0;
    int cond = 0;
    int ret = 0;
    int size = 0;
    SuperBlock superBlock;
    int fatherInodeOffset = 0;
    int destInodeOffset = 0;
    Inode fatherInode;
    Inode destInode;
    if (driver == NULL) {
        printf("driver == NULL.\n");
        return -1;
    }
    file = fopen(driver, "r+");
    if (file == NULL) {
        printf("Failed to open driver.\n");
        return -1;
    }
    ret = readSuperBlock(file, &superBlock);
    if (ret == -1) {
        printf("Failed to load SuperBlock.\n");
        fclose(file);
        return -1;
    }
    if (destDirPath == NULL) {
        printf("destDirPath == NULL");
        fclose(file);
        return -1;
    }
    length = stringLen(destDirPath);
    if (destDirPath[length - 1] == '/') {
        cond = 1;
        *((char*)destDirPath + length - 1) = 0;
    }
    ret = stringChrR(destDirPath, '/', &size);
    if (ret == -1) {
        printf("Incorrect destination file path.\n");
        fclose(file);
        return -1;
    }
    tmp = *((char*)destDirPath + size + 1);
    *((char*)destDirPath + size + 1) = 0;
    ret = readInode(file, &superBlock, &fatherInode, &fatherInodeOffset, destDirPath);
    *((char*)destDirPath + size + 1) = tmp;
    if (ret == -1) {
        printf("Failed to read father inode.\n");
        if (cond == 1)
            *((char*)destDirPath + length - 1) = '/';
        fclose(file);
        return -1;
    }
    ret = allocInode(file, &superBlock, &fatherInode, fatherInodeOffset,
        &destInode, &destInodeOffset, destDirPath + size + 1, DIRECTORY_TYPE);
    if (ret == -1) {
        printf("Failed to allocate inode.\n");
        if (cond == 1)
            *((char*)destDirPath + length - 1) = '/';
        fclose(file);
        return -1;
    }
    ret = initDir(file, &superBlock, &fatherInode, fatherInodeOffset,
        &destInode, destInodeOffset);
    if (ret == -1) {
        printf("Failed to initialize dir.\n");
        if (cond == 1)
            *((char*)destDirPath + length - 1) = '/';
        fclose(file);
        return -1;
    }
    if (cond == 1)
        *((char*)destDirPath + length - 1) = '/';
    
    printf("mkdir %s\n", destDirPath);
    printf("MKDIR success.\n%d inodes and %d data blocks available.\n", superBlock.availInodeNum, superBlock.availBlockNum);
    fclose(file);
    return 0;
}

int rmdir (const char *driver, const char *destDirPath) {
    // TODO
    return 0;
}

/*
 *  Supported destFilePath pattern "/xxx/.../xxxx".
 */
int cp (const char *driver, const char *srcFilePath, const char *destFilePath) {
    FILE *file = NULL;
    FILE *fileSrc = NULL;
    char tmp = 0;
    int ret = 0;
    int size = 0;
    int length = 0;
    SuperBlock superBlock;
    int fatherInodeOffset = 0;  // byte as unit
    int destInodeOffset = 0;    // byte as unit
    Inode fatherInode;          // Inode for dir
    Inode destInode;            // Inode for name

    if (driver == NULL || srcFilePath == NULL) {
        printf("driver == NULL || srcFilePath == NULL.\n");
        return -1;
    }
    file = fopen(driver, "r+");
    if (file == NULL) {
        printf("Failed to open driver.\n");
        return -1;
    }
    fileSrc = fopen(srcFilePath, "r");
    if (fileSrc == NULL) {
        printf("Failed to open srcFilePath.\n");
        fclose(file);
        return -1;
    }
    ret = readSuperBlock(file, &superBlock);
    if (ret == -1) {
        printf("Failed to load superBlock.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    if (destFilePath == NULL) {
        printf("destFilePath == NULL.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    length = stringLen(destFilePath);
    if (destFilePath[0] != '/' || destFilePath[length - 1] == '/') {
        printf("Incorrect destination file path.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    ret = stringChrR(destFilePath, '/', &size);
    if (ret == -1) { // no '/' in destFilePath
        printf("Incorrect destination file path.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    tmp = *((char*)destFilePath + size + 1);
    *((char*)destFilePath + size + 1) = 0;  // destFilePath is dir ended with '/'.
    ret = readInode(file, &superBlock, &fatherInode, &fatherInodeOffset, destFilePath);
    *((char*)destFilePath + size + 1) = tmp;
    if (ret == -1) {
        printf("Failed to read father inode.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    ret = allocInode(file, &superBlock, &fatherInode, fatherInodeOffset,
        &destInode, &destInodeOffset, destFilePath + size + 1, REGULAR_TYPE);
    if (ret == -1) {
        printf("Failed to allocate inode.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    ret = copyData(file, fileSrc, &superBlock, &destInode, destInodeOffset);
    if (ret == -1) {
        printf("Failed to copy data.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    printf("cp %s %s\n", srcFilePath, destFilePath);
    printf("CP success.\n%d inodes and %d data blocks available.\n", superBlock.availInodeNum, superBlock.availBlockNum);
    fclose(file);
    fclose(fileSrc);
    return 0;
}

int rm (const char *driver, const char *destFilePath) {
    // TODO
    return 0;
}

int ls (const char *driver, const char *destFilePath) {
    // definition
    FILE *file = NULL;
    int ret = 0;
    SuperBlock superBlock;
    Inode inode;
    int inodeOffset = 0;
    DirEntry dirEntry;
    int dirIndex = 0;
    Inode childInode;
    int inodeIndex = 0;

    if (driver == NULL) {
        printf("driver == NULL.\n");
        return -1;
    }
    if (destFilePath == NULL) {
        printf("destFilePath == NULL.\n");
        return -1;
    }
    file = fopen(driver , "r");
    if (file == NULL) {
        printf("Failed to open driver.\n");
        return -1;
    }
    ret = readSuperBlock(file, &superBlock);
    if (ret == -1) {
        printf("Failed to load superBlock.\n");
        fclose(file);
        return -1;
    }
    ret = readInode(file, &superBlock, &inode, &inodeOffset, destFilePath);
    if (ret == -1) {
        printf("Failed to read inode.\n");
        fclose(file);
        return -1;
    }
    if (inode.type == REGULAR_TYPE) {
        printf("ls %s\n", destFilePath);
        inodeIndex = (inodeOffset - superBlock.inodeTable) / sizeof(Inode) + 1;
        printf("Inode: %d, Type: %d, LinkCount: %d, BlockCount: %d, Size: %d.\n",
            inodeIndex, inode.type, inode.linkCount, inode.blockCount, inode.size);
        fclose(file);
        return 0;
    }
    printf("ls %s\n", destFilePath);
    while (getDirEntry(file, &superBlock, &inode, dirIndex, &dirEntry) == 0) {
        dirIndex ++;
        fseek(file, superBlock.inodeTable * SECTOR_SIZE + (dirEntry.inode - 1) * sizeof(Inode), SEEK_SET);
        fread((void*)&childInode, sizeof(Inode), 1, file);
        printf("Name: %s, Inode: %d, Type: %d, LinkCount: %d, BlockCount: %d, Size: %d.\n",
            dirEntry.name, dirEntry.inode, childInode.type, childInode.linkCount, childInode.blockCount, childInode.size);
    }
    printf("LS success.\n%d inodes and %d data blocks available.\n", superBlock.availInodeNum, superBlock.availBlockNum);
    fclose(file);
    return 0;
}

int touch (const char *driver, const char *destFilePath) {
    FILE *file = NULL;
    FILE *fileSrc = NULL;
    char tmp = 0;
    int ret = 0;
    int size = 0;
    int length = 0;
    SuperBlock superBlock;
    int fatherInodeOffset = 0;  // byte as unit
    int destInodeOffset = 0;    // byte as unit
    Inode fatherInode;          // Inode for dir
    Inode destInode;            // Inode for name

    if (driver == NULL) {
        printf("driver == NULL.\n");
        return -1;
    }
    file = fopen(driver, "r+");
    if (file == NULL) {
        printf("Failed to open driver.\n");
        return -1;
    }
    ret = readSuperBlock(file, &superBlock);
    if (ret == -1) {
        printf("Failed to load superBlock.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    if (destFilePath == NULL) {
        printf("destFilePath == NULL.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    length = stringLen(destFilePath);
    if (destFilePath[0] != '/' || destFilePath[length - 1] == '/') {
        printf("Incorrect destination file path.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    ret = stringChrR(destFilePath, '/', &size);
    if (ret == -1) { // no '/' in destFilePath
        printf("Incorrect destination file path.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    tmp = *((char*)destFilePath + size + 1);
    *((char*)destFilePath + size + 1) = 0;  // destFilePath is dir ended with '/'.
    ret = readInode(file, &superBlock, &fatherInode, &fatherInodeOffset, destFilePath);
    *((char*)destFilePath + size + 1) = tmp;
    if (ret == -1) {
        printf("Failed to read father inode.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    ret = allocInode(file, &superBlock, &fatherInode, fatherInodeOffset,
        &destInode, &destInodeOffset, destFilePath + size + 1, REGULAR_TYPE);
    if (ret == -1) {
        printf("Failed to allocate inode.\n");
        fclose(file);
        fclose(fileSrc);
        return -1;
    }
    printf("touch %s\n", destFilePath);
    printf("TOUCH success.\n%d inodes and %d data blocks available.\n", superBlock.availInodeNum, superBlock.availBlockNum);
    fclose(file);
    return 0;
}
