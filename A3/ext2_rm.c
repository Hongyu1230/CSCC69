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
    strcpy(destpath, argv[2]);
    strcpy(destpath2, argv[2]);
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
    while (token2 != NULL && S_ISDIR(pathnode->i_mode) && startpoint < stoppoint - 1) {
        startpoint +=1;
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
    check = 0;
    struct ext2_dir_entry_2 *directorycheck;
    struct ext2_inode *deletionnode;
    struct ext2_dir_entry_2 *deletiondirectory;
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (pathnode->i_block[blockpointer] == 0){
            break;
        }
        lengthcomp = strlen(filename);
        directorycheck = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < pathnode->i_size) {
            if(strncmp(filename, directorycheck->name, directorycheck->name_len) == 0 && lengthcomp == directorycheck->name_len) {
                if (directorycheck->file_type == 2) {
                    perror("the file at the location is a directory");
                    return EEXIST;
                } else {
                    check = 1;
                    break;
                }
            } else {
                if (directorycheck->rec_len == 0) {
                    break;
                }
                sizecheck += directorycheck->rec_len;
                directorycheck = (void *) directorycheck + directorycheck->rec_len;
            }
        }
        //we found the file we need to delete
        if (check == 1) {
            deletionnode = itable + directorycheck->inode - 1;
            deletiondirectory = (struct ext2_dir_entry_2 *)(disk + 1024 * deletionnode->i_block[blockpointer]);
            break;
        }
    }
    
    if (check == 0) {
        perror("cannot find the file at the location given");
        return ENOENT;
    }
    
    
    int inode_bitmap[32];
    char *ibmap = (char *)(disk + 1024 * bg->bg_inode_bitmap);
    int i, pos;
    char temp;
    for (i = 0; i < 4; i+=1, ibmap +=1) {
        temp = *ibmap;
        for (pos = 0; pos < 8; pos+=1) {
            inode_bitmap[(8 * i) + pos] = (temp >> pos) & 1;
        }
    }
    
    int block_bitmap[128];
    char *bbmap = (char *)(disk + 1024 * bg->bg_block_bitmap);
    for (i = 0; i < 16; i+=1, bbmap +=1) {
        temp = *bbmap;
        for (pos = 0; pos < 8; pos+=1) {
            block_bitmap[(8 * i) + pos] = (temp >> pos) & 1;
        }
    }
    int n;
    deletionnode->i_links_count -= 1;
    int blockfreed = 0;
    //zero out the bitmap for inode and blocks for our operation later
    for (i = 0; i < 12; i +=1){
        if (deletionnode->i_block[i] == 0) {
            break;
        } else {
            blockfreed += 1;
            block_bitmap[deletionnode->i_block[i] - 1] = 0;
        }   
    }
    
    struct ext2_dir_entry_2 *oldentry;
    int spaceold, oldsize, unusedblock, oldlen;
    check = 0;
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (pathnode->i_block[blockpointer] == 0){
            unusedblock = blockpointer;
            break;
        }
        oldentry = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < 1024) {
			printf("%s\n", deletionfile->name);
            if (oldentry == deletiondirectory) {
                check = 1;
                //we clearly have a entry before this
                if (sizecheck > 0) {
					printf("wait does this work");
                    oldentry = (void *) oldentry - oldlen;
                    oldentry->rec_len += deletiondirectory->rec_len;
                    break;
                } else {
                }
            } else {
                oldentry = (void *) oldentry + oldentry->rec_len;
                oldlen = oldentry->rec_len;
            }
            sizecheck += oldentry->rec_len;
        }
        if (check == 1){
            break;
        }
    }
    
    if (deletionnode->i_links_count == 0) {
        bbmap = (char *)(disk + 1024 * bg->bg_block_bitmap);
        for (i = 0; i < 16; i+=1, bbmap +=1) {
            for (pos = 0; pos < 8; pos+=1) {
                if (block_bitmap[(8 * i) + pos] == 1) {
                    *bbmap |= (int) pow(2,pos);
                }
            }
        }   
        ibmap = (char *)(disk + 1024 * bg->bg_inode_bitmap);
        for (i = 0; i < 4; i+=1, ibmap +=1) {
            for (pos = 0; pos < 8; pos+=1) {
                if (inode_bitmap[(8 * i) + pos] == 1) {
                    *ibmap |= (int) pow(2,pos);
                }
            }
        }
        sb->s_free_blocks_count += blockfreed;
        sb->s_free_inodes_count += 1;
    }
    
    return 0;
}