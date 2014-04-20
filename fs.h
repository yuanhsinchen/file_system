#include <stdint.h>

/* Each descriptor is 1024 Byte which is the same as block size */

typedef struct file_descriptor {
  char fname[256]; /* filename */
  int fsize; /* file size */
  uint16_t bid[382]; /* block id of the file */
} file_desc;

struct entry {
  uint16_t bid;
  uint8_t type; /* type of entry: file=1 or directory=0 */
};

typedef struct dir_descriptor {
  char dname[256]; /* directory name */
  int dnum; /* how many files and directories in it? */
  uint16_t parbid; /* parent's bid */
  struct entry e[190]; /* entry block of the files and directories */
} dir_desc;
    
typedef struct superblock {
  int root_bid; /* block id of root directory */
  int fs_size; /* size of the file system */
  /* 40 * 1024 blocks means 40 * 1024 bits for bitmap */
  /* 5 blocks needed */
  /* bitmap bids are 1 - 5 */
} superblock;

typedef struct block {
  uint8_t block[1024];
} block;
