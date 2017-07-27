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


int main(int argc, char **argv) {

    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_rm <image file name> <FILE>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);
    char destpath[strlen(argv[2])];
	char destpath2[strlen(argv[2])];
    strcpy(destpath, argv[3]);
	strcpy(destpath2, argv[3]);
    if (destpath[0] != '/') {
        perror("the path needs to start from root, beginning with /");
        return ENOENT;
    }
    char *token;
    char *token2;
    const char delimiter[2] = "/";
    char destinationsplit[strlen(destpath)];
    token = strtok(destpath, delimiter);
	char filename[strlen(destpath)];
	int stoppoint = 0;
    while (token != NULL) {
        strcpy(filename, token);
		stoppoint +=1;
        token = strtok(NULL, delimiter);
    }
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_inode *itable = (struct ext2_inode *)(disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *pathnode = itable + 1;
    token2 = strtok(destpath2, delimiter);
    int sizecheck, check, blockpointer, found, lengthcomp, startpoint = 0;
    struct ext2_dir_entry_2 *directory;
    while (token2 != NULL && S_ISDIR(pathnode->i_mode) && startpoint < stoppoint) {
		stoppoint +=1;
        lengthcomp = strlen(token2);
        for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
			if (pathnode->i_block[blockpointer] == 0){
			    break;
		    }
            directory = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
            sizecheck = 0;
            while (sizecheck < pathnode->i_size) {
                if(strncmp(token2, directory->name, directory->name_len) == 0 && lengthcomp == directory->name_len) {
                    pathnode = itable + directory->inode - 1;
                    sizecheck = 0;
                    check = 1;
                    found = 1;
                    token2 = strtok(NULL, delimiter);
                    break;
                } else {
                    if (directory->rec_len == 0) {
                        break;
                    }
                    sizecheck += directory->rec_len;
                    directory = (void *) directory + directory->rec_len;
                }
            }
            if (check == 1) {
                check = 0;
                break;
            } 
        }
        if (found == 1) {
            found = 0;
        } else {
            perror("cannot one of the files on the file path");
            return ENOENT;
        }
    }
	
    
    
    
    
    return 0;
}