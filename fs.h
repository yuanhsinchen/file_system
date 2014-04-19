#include <stdint.h>

/* Each descriptor is 1024 Byte which is the same as block size */

typedef struct file_descriptor {
char fname[64]; /* filename */
int fsize; /* file size */
uint16_t bid[30]; /* block id of the file */
}fd;

struct entry {
uint16_t bid;
uint8_t type; /* type of entry: file=1 or directory=0 */
};

typedef struct dir_descriptor {
char dname[64]; /* directory name */
int dnum; /* how many files and directories in it? */
struct entry e[15]; /* entry block of the files and directories */
}dd;
    
typedef struct superblock {
int root_bid; /* block id of root directory */
int fs_size; /* size of the file system */
uint32_t bitmap; /* bitmap of whether a block is occupied */
}sb;
