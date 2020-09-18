#ifndef __MINIX_H__
#define __MINIX_H__

/*
 *  minix like filesystem
 */

/*
 *                                                        Layout of file system
 *+--------------------+-------------------+--------------------+--------------------+--------------------+
 *|     SuperBlock     |    InodeBitmap    |    BlockBitmap     |     InodeTable     |     DataBlocks     |
 *+--------------------+-------------------+--------------------+--------------------+--------------------+
 *      1024 Bytes             1 Block             1 Block             128 Blocks        Many More Blocks
 *
 */

#define SECTORS_PER_BLOCK 2
#define POINTER_NUM 14
#define NAME_LENGTH 64

#define BLOCK_SIZE (SECTOR_SIZE * SECTORS_PER_BLOCK)
#define SUPER_BLOCK_SIZE BLOCK_SIZE
#define INODE_BITMAP_SIZE BLOCK_SIZE
#define BLOCK_BITMAP_SIZE BLOCK_SIZE
#define INODE_SIZE 128
#define DIRENTRY_SIZE 128
#define INODE_BLOCKS (SECTOR_NUM / SECTORS_PER_BLOCK / 32)

#define UNKNOWN_TYPE 0
#define REGULAR_TYPE 1
#define DIRECTORY_TYPE 2
#define CHARACTER_TYPE 3
#define BLOCK_TYPE 4
#define FIFO_TYPE 5
#define SOCKET_TYPE 6
#define SYMBOLIC_TYPE 7

union SuperBlock {
    uint8_t byte[SUPER_BLOCK_SIZE];
    struct {
        int32_t sectorNum;      // total number of sectors
        int32_t inodeNum;       // total number of inodes
        int32_t blockNum;       // total number of data blocks
        int32_t availInodeNum;  // total number of available inodes
        int32_t availBlockNum;  // total number of available data blocks
        int32_t blockSize;      // number of bytes in each block
        int32_t inodeBitmap;    // XXX sector as unit
        int32_t blockBitmap;    // XXX sector as unit
        int32_t inodeTable;     // XXX sector as unit
        int32_t blocks;         // XXX sector as unit
    };
};
typedef union SuperBlock SuperBlock;

struct InodeBitmap {
    uint8_t byte[INODE_BITMAP_SIZE];
};
typedef struct InodeBitmap InodeBitmap;

struct BlockBitmap {
    uint8_t byte[BLOCK_BITMAP_SIZE];
};
typedef struct BlockBitmap BlockBitmap;

union Inode {
    uint8_t byte[INODE_SIZE];
    struct {
        int16_t type;
        int16_t linkCount;
        int32_t blockCount;
        int32_t size;
        int32_t pointer[POINTER_NUM];   // XXX sector as unit
        int32_t singlyPointer;          // XXX sector as unit
    };
};
typedef union Inode Inode;

union DirEntry {
    uint8_t byte[DIRENTRY_SIZE];
    struct {
        int32_t inode;              // index in inode table, started from 1, 0 for unused.
        char name[NAME_LENGTH];
    };
};
typedef union DirEntry DirEntry;

#endif
