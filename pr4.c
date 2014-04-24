/* CMPSC 473, Project 4, starter kit
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "fs.h"

/*--------------------------------------------------------------------------------*/

int debug = 0;  // extra output; 1 = on, 0 = off
void *disk = NULL;
int cwd;
/*--------------------------------------------------------------------------------*/

/* The input file (stdin) represents a sequence of file-system commands,
 * which all look like     cmd filename filesize
 *
 * command  action
 * -------  ------
 *  root    initialize root directory
 *  print   print current working directory and all descendants
 *  chdir   change current working directory
 *                (.. refers to parent directory, as in Unix)
 *  mkdir   sub-directory create (mk = make)
 *  rmdir                 delete (rm = delete)
 *  mvdir                 rename (mv = move)
 *  mkfil   file create
 *  rmfil        delete
 *  mvfil        rename
 *  szfil        resize (sz = size)
 *  exit        quit the program immediately
 */

/* The size argument is usually ignored.
 * The return value is 0 (success) or -1 (failure).
 */
int do_root (char *name, char *size);
int do_print(char *name, char *size);
int do_chdir(char *name, char *size);
int do_mkdir(char *name, char *size);
int do_rmdir(char *name, char *size);
int do_mvdir(char *name, char *size);
int do_mkfil(char *name, char *size);
int do_rmfil(char *name, char *size);
int do_mvfil(char *name, char *size);
int do_szfil(char *name, char *size);
int do_exit (char *name, char *size);

struct action {
    char *cmd;                    // pointer to string
    int (*action)(char *name, char *size);    // pointer to function
} table[] = {
    { "root" , do_root  },
    { "print", do_print },
    { "chdir", do_chdir },
    { "mkdir", do_mkdir },
    { "rmdir", do_rmdir },
    { "mvdir", do_mvdir },
    { "mkfil", do_mkfil },
    { "rmfil", do_rmfil },
    { "mvfil", do_mvfil },
    { "szfil", do_szfil },
    { "exit" , do_exit  },
    { NULL, NULL }  // end marker, do not remove
};

/*--------------------------------------------------------------------------------*/

void parse(char *buf, int *argc, char *argv[]);

#define LINESIZE 128
#define DISKSIZE (40*1024*1024)
#define BLOCKSIZE 1024
#define BLOCKSIZEWORD (1024 / 4)
#define BLOCKNUM (DISKSIZE / BLOCKSIZE) /* how many blocks */
#define BITMAPSIZEWORD (DISKSIZE / BLOCKSIZE / 32)

/*--------------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
    char in[LINESIZE];
    char *cmd, *fnm, *fsz;
    char dummy[] = "";
    
    int n;
    char *a[LINESIZE];
    
    while (fgets(in, LINESIZE, stdin) != NULL) {
        // commands are all like "cmd filename filesize\n" with whitespace between
        
        // parse in
        parse(in, &n, a);
        
        cmd = (n > 0) ? a[0] : dummy;
        fnm = (n > 1) ? a[1] : dummy;
        fsz = (n > 2) ? a[2] : dummy;
        if (debug) printf(":%s:%s:%s:\n", cmd, fnm, fsz);
        
        if (n == 0) continue; // blank line
        
        int found = 0;
        for (struct action *ptr = table; ptr->cmd != NULL; ptr++) {
            if (strcmp(ptr->cmd, cmd) == 0) {
                found = 1;
                int ret = (ptr->action)(fnm, fsz);
                if (ret == -1) {
                    printf("  %s %s %s: failed\n", cmd, fnm, fsz);
                }
                break;
            }
        }
        if (!found) {
            printf("command not found: %s\n", cmd);
        }
    }
    
    return 0;
}

/*--------------------------------------------------------------------------------*/
void clear_bit(uint32_t *bitmap, int n) {
    int word = n / 32;
    int bit = n % 32;
    
    bitmap[word] &= ~(1 << bit);
}

void set_bit(uint32_t *bitmap, int n) {
    int word = n / 32;
    int bit = n % 32;
    
    bitmap[word] |= 1 << bit;
}

uint16_t empty_bid(uint32_t *bitmap) {
    int i, j;
    for (i = 0; i < BITMAPSIZEWORD; i++) {
        if ((bitmap[i] == 0xFFFFFFFF))
            continue;
        for (j = 0; j < 32; j++) {
            if (!(bitmap[i] & (1 << j)))
                return (i * 32 + j);
        }
    }
    /* file system is full */
    return 0;
    
}
/*--------------------------------------------------------------------------------*/
void print_bitmap(uint32_t *bitmap, int n) {
    int i;
    printf("print_bitmap");
    for (i = 0; i < n; i++) {
        if (i % 4 == 0)
            printf("\n");
        printf("%x  ", bitmap[i]);
    }
    printf("\n");
}

void print_block(void *disk, int bid) {
    int i;
    uint8_t b[BLOCKSIZE];
    
    memcpy(b, &((uint8_t *)disk)[bid * BLOCKSIZE], BLOCKSIZE);
    
    printf("print_block %d", bid);
    for (i = 0; i < BLOCKSIZE; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%x ", b[i]);
    }
    printf("\n");
}

/*--------------------------------------------------------------------------------*/
void write_block(void *data, int bid) {
    uint8_t *b = &((uint8_t *)disk)[bid * BLOCKSIZE];
    memcpy(b, data, BLOCKSIZE);
}

void read_block(void *data, int bid) {
    uint8_t *b = &((uint8_t *)disk)[bid * BLOCKSIZE];
    memcpy(data, b, BLOCKSIZE);
}

void update_parent(dir_desc *update, int file_or_dir, int bid) {
    for (int i = 0; i < 190; i++) {
        if (update->e[i].bid == 0) {
            update->e[i].bid = bid;
            update->e[i].type = file_or_dir;
            return;
        }
    }
    
    fprintf(stderr, "ERROR: NOT ENOUGH SPACE IN DIRECTORY\n");
}

void add_block(file_desc *file, int bid){
    for(int i=0; i<382; i++){
        if(file->bid[i] == 0) {
            file->bid[i] = bid;
            printf ("wrote %d to file[%d]\n", bid, i);
            return;
        }
    }
    
    fprintf(stderr, "ERROR: %s TOO LARGE", file->fname);
}

//the next two functions are wholly unncessary but i wrote both of them
//before i found the function atoi() so i'm keeping them out of spite.
int exp10(int pow){
    int value=10;
    if(pow == 0)
        return 1;
    else{
        
        for(int i=1; i < pow; i++){
            value *= 10;
        }
    }
    
    return value;
}

int parse_char_to_int(char *number){
    int int_size=0;
    int count=0;
    for(int i=0; number[i] != '\0'; i++){
        count++;
    }
    
    int digit = count-1;
    for(int j=0; j<count; j++){
        int_size += (number[j]-'0')*exp10(digit);
        digit--;
    }
    
    return int_size;
}
/*--------------------------------------------------------------------------------*/

int do_root(char *name, char *size) {
    cwd = 6;
    superblock sb;
    uint32_t bitmap[BITMAPSIZEWORD];
    dir_desc root;
    int i;
    
    /* initialize superblock */
    disk = malloc(DISKSIZE);
    if (disk == NULL) {
        printf("disk allocation failed\n");
        exit (1);
    }
    memset(bitmap, 0, sizeof(uint32_t) * BITMAPSIZEWORD);
    set_bit(bitmap, 0); /* block 0 for superblock */
    
    sb.fs_size = DISKSIZE;
    
    /* block 1-5 for bitmap */
    for (i = 1; i <= 5; i++)
        set_bit(bitmap, i);
    
    /* block 6 for root directory */
    memset(&root, 0, sizeof(dir_desc));
    strcpy(root.dname, "root");
    set_bit(bitmap, 6);
    root.parbid = 6;
    
    sb.root_bid = 6;
    
    /* write descriptors to blocks*/
    write_block(&sb, 0);
    for (i = 0; i < 5; i++)
        write_block(&bitmap[i * BLOCKSIZEWORD], i + 1);
    //print_block(disk, 1);
    write_block(&root, 6);
    
    if (debug) printf("%s\n", __func__);
    return 0;
}

int do_print(char *name, char *size) {
    superblock sb;
    dir_desc root;
    
    read_block(&sb, 0);
    read_block(&root, sb.root_bid);
    
    if (debug) printf("%s\n", __func__);
    return 0;
}

int do_chdir(char *name, char *size) {
    
    uint32_t bitmap[BITMAPSIZEWORD];
    dir_desc cwdb, todb;
    read_block( &cwdb, cwd);
    for (int i = 0; i < 5; i++) {
      read_block(&bitmap[i * BLOCKSIZEWORD], i + 1);
    }
    if (!strcmp(name, "..")) {
      cwd = cwdb.parbid;
      read_block( &cwdb, cwd);
    }
    else {
      int find_dir = 0;
      for (int i = 0; i < 190; i++){
        if (cwdb.e[i].bid > 0) {
          int t_a = cwdb.e[i].bid / 32;
          int t_b = cwdb.e[i].bid % 32;
          if ((bitmap[t_a] & (1 << t_b))) {
//            printf("block %d is under this directory \n", cwdb.e[i].bid);
            if (cwdb.e[i].type == 0) {
              read_block( &todb, cwdb.e[i].bid);
//              printf("it is a directory (%s)\n", todb.dname);
              if (!strcmp(todb.dname, name)) {
                find_dir = 1;
                cwd = cwdb.e[i].bid;
                read_block( &cwdb, cwd);
              }
            }
          }
        }
      }
      if (find_dir == 0) {
        printf("Directory '%s' not found.\n", name);
      }
    }
    printf("\n%s\\>", cwdb.dname);
    
//    printf("CD %d = (%d, %d)\n", to_dir, to_dir_block, to_dir_bit);
    
    if (debug) printf("%s\n", __func__);
    return 0;

}

int do_mkdir(char *name, char *size) {
    superblock *sb;
    uint32_t bitmap[BITMAPSIZEWORD];
    int empty_block = 0;
    int i;
    dir_desc current_dir;
    
    //find an open block
    for (i = 1; i <= 5; i++) {
        read_block(bitmap, i);
        empty_block = empty_bid(bitmap);
        if (empty_block != 0) //if empty block is found, break
            break;
    }
    
    set_bit(bitmap, empty_block); //set bit in bitmap to 1
    dir_desc new_dir_desc; //new dir_desc
    strcpy(new_dir_desc.dname, name); //set dir_desc name
    new_dir_desc.dnum = 0; //set number of directories/files
    new_dir_desc.parbid = cwd; //set parent bid
    
    write_block(&new_dir_desc, empty_block); //write new dir_desc to empty block
    write_block(bitmap, i); //write updated bitmap to bitmap block
    
    read_block(&current_dir, cwd); //read current_dir
    update_parent(&current_dir, 0, empty_block); //update parent
    current_dir.dnum++;
    //printf("%d, %d", current_dir.e[0].bid, current_dir.e[0].type); //check
    write_block(&current_dir, cwd); //write back to block
    
    
    if (debug) printf("%s\n", __func__);
    return 0;
}

int do_rmdir(char *name, char *size) {

    if (debug) printf("%s\n", __func__);
    return -1;
}

int do_mvdir(char *name, char *size) {
    if (debug) printf("%s\n", __func__);
    return -1;
}

// TODO: check file size, if file is bigger than block store it on multiple blocks
int do_mkfil(char *name, char *size) {
    superblock *sb;
    uint32_t bitmap[BITMAPSIZEWORD];
    uint16_t empty_block = 0;
    dir_desc current_dir;
    int file_size = atoi(size);
    int number_of_blocks;
    int file_desc_block;
    file_desc new_file_desc;

    int k=1;
    int j;
    int i;

    if(size[0] == '\0')
        file_size = 0;
    number_of_blocks = 1 + ((file_size - 1) / BLOCKSIZE);
    
    strcpy(new_file_desc.fname, name);
    new_file_desc.fsize = file_size;
    memset(new_file_desc.bid, 0, 381);
    
    for (i = 1; i <= 5; i++) {
        read_block(bitmap, i);
        file_desc_block = empty_bid(bitmap);
        if (file_desc_block != 0){
            write_block(&new_file_desc, file_desc_block);
            set_bit(bitmap, file_desc_block);
            write_block(bitmap, i);
            break;
        }
    }
    
    
    for (j = i; j <= 5; j++) {
        read_block(bitmap, j);
        while(k <= number_of_blocks && k < BITMAPSIZEWORD) {
            empty_block = empty_bid(bitmap);
            if (empty_block != 0){
                set_bit(bitmap, empty_block);
                add_block(&new_file_desc, empty_block);
                k++;
            }
        }

        if(k-1 == number_of_blocks){
            write_block(bitmap, j);
            print_bitmap(bitmap, 20);
            break;
        }
    }
    
    read_block(&current_dir, cwd); //read current_dir
    update_parent(&current_dir, 1, file_desc_block);    //update parent
    current_dir.dnum++;
    //printf("%d, %d", current_dir.e[0].bid, current_dir.e[0].type); //check
    write_block(&current_dir, cwd);
        
    if (debug) printf("%s\n", __func__);
    return 0;
}

int do_rmfil(char *name, char *size) {
    if (debug) printf("%s\n", __func__);
    return -1;
}

int do_mvfil(char *name, char *size) {
    if (debug) printf("%s\n", __func__);
    return -1;
}

int do_szfil(char *name, char *size) { //christina
    
    /* find file
     calculate blocks needed/not needed
     change block bitmap
     update file descriptor
     SZFIL ONLY WORKS ON FILES IN CURRENT DIRECTORY
     */
    
    dir_desc current_dir;
    file_desc temp_block_id;
    
    
    //parse size into int
    int size_of_file = atoi(size);
    int found=0;
    
    //read files from directory
    read_block(&current_dir, cwd);
    for(int i=0; i < 190; i++){
        if(current_dir.e[i].type == 1){
            read_block(&temp_block_id, current_dir.e[i].bid);
            if(strcmp(temp_block_id.fname, name) == 0) {
                found=1;
                break;
            }
        }
    }
    if (found == 0) {
        printf("%s was not found\n", name);
        return -1;
    }
    
    //calculate number of blocks needed
    int num_of_blocks = 1 + ((size_of_file - 1) / BLOCKSIZE);
    int k=1;
    uint32_t bitmap[BITMAPSIZEWORD];
    uint16_t empty_block = 0;
    
    for(int i=0; i < 382; i++){
        printf("%d ", temp_block_id.bid[i]);
    }
    
    for (int j = 1; j <= 5; j++) {
        read_block(bitmap, j);
        while(k <= num_of_blocks && k < BITMAPSIZEWORD) {
            empty_block = empty_bid(bitmap);
            if (empty_block != 0){
                set_bit(bitmap, empty_block);
                //print_bitmap(bitmap, 20);
                add_block(&temp_block_id, empty_block);
                k++;
            }
        }
        if(k == num_of_blocks){
            break;
            write_block(bitmap, j);
        }
    }
    
    if (debug) printf("%s\n", __func__);
    return 0;
}

int do_exit(char *name, char *size) {
    free(disk);
    if (debug) printf("%s\n", __func__);
    exit(0);
    return 0;
}

/*--------------------------------------------------------------------------------*/

// parse a command line, where buf came from fgets()

// Note - the trailing '\n' in buf is whitespace, and we need it as a delimiter.

void parse(char *buf, int *argc, char *argv[]) {
    char *delim;          // points to first space delimiter
    int count = 0;        // number of args
    
    char whsp[] = " \t\n\v\f\r";          // whitespace characters
    
    while (1) {                           // build the argv list
        buf += strspn(buf, whsp);         // skip leading whitespace
        delim = strpbrk(buf, whsp);       // next whitespace char or NULL
        if (delim == NULL) {              // end of line
            break;
        }
        argv[count++] = buf;              // start argv[i]
        *delim = '\0';                    // terminate argv[i]
        buf = delim + 1;                  // start argv[i+1]?
    }
    argv[count] = NULL;
    
    *argc = count;
    
    return;
}

/*--------------------------------------------------------------------------------*/

