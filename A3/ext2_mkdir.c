#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include "ext2.h"

unsigned char *disk;

//most of the code here is just copied from cp, since I did that first
int main(int argc, char **argv) {

    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_mkdir <image file name> <DEST>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);
    char destpath[strlen(argv[2])];
    strcpy(destpath, argv[2]);
    if (destpath[0] != '/') {
        perror("the path needs to start from root, beginning with /");
        return ENOENT;
    }
    char *token;
    char *token2;
    const char delimiter[2] = "/";
    char destinationsplit[strlen(destpath)];
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    int blockneeded = 1;
    
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    
    if (blockneeded > sb->s_free_blocks_count) {
        perror("not enough space for the new file");
        return ENOSPC;
    }
	char *storedarray[strlen(destpath)];
	char filename[strlen(destpath)];
	int pathlocation = 0;
    token = strtok(destpath, delimiter);
    
    
    
    
    
    
    return 0;
}