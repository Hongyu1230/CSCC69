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
    char destpath[strlen(argv[2]) + 1];
    //for the 2nd strtok call
    char destpath2[strlen(argv[2]) + 1];
    strcpy(destpath, argv[2]);
    strcpy(destpath2, argv[2]);
    if (destpath[0] != '/') {
        printf("the path needs to start from root, beginning with /\n");
        return ENOENT;
    }
    if (strlen(destpath) == 1) {
        printf("you need to specify a path with at least a directory to create\n");
        return ENOENT;
    }
    char *token;
    char *token2;
    const char delimiter[2] = "/";
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        printf("mmap");
        exit(1);
    }
    
    int blockneeded = 1;
    int blockused = 0;
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    //want to make sure we have 1 extra block in case we need one for inserting a directory
    if (blockneeded + 1 > sb->s_free_blocks_count) {
        printf("not enough space for the new file\n");
        return ENOSPC;
    }
    char filename[strlen(destpath) + 1];
    int pathlocation = 0;
    token = strtok(destpath, delimiter);
    //finding the name of our new directory
    while (token != NULL) {
        strcpy(filename, token);
        pathlocation += 1;
        token = strtok(NULL, delimiter);
    }
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_inode *itable = (struct ext2_inode *)(disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *pathnode = itable + 1;
    token2 = strtok(destpath2, delimiter);
    int parentnode;
    int sizecheck, check = 0, blockpointer, found = 0, lengthcomp, startingpoint = 0, passedonce = 0;
    struct ext2_dir_entry_2 *directory;
    //traverse to the 2nd last path to know the parent
    while (token2 != NULL && startingpoint < pathlocation - 1) {
        passedonce = 1;
        startingpoint += 1;
        lengthcomp = strlen(token2);
        for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
            if (pathnode->i_block[blockpointer] == 0){
                printf("cannot find destination directory on disk\n");
                return ENOENT;
            }
            directory = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
            sizecheck = 0;
            while (sizecheck < 1024) {
                if(strncmp(token2, directory->name, directory->name_len) == 0 && lengthcomp == directory->name_len) {
                    pathnode = itable + directory->inode - 1;
                    parentnode = directory->inode;
                    if (!(S_ISDIR(pathnode->i_mode))) {
                        printf("one of the files on the path is not a directory\n");
                        return ENOENT;
                    }
                    sizecheck = 0;
                    check = 1;
                    found = 1;
                    token2 = strtok(NULL, delimiter);
                    break;
                } else {
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
            printf("cannot find destination directory on disk\n");
            return ENOENT;
        }
    }
    
    //our parent node is the root since we never gone through the traverse
    if (passedonce == 0) {
        parentnode = 2;
    }
    
    //make sure we don't have another file with the same name in the parent directory
    struct ext2_dir_entry_2 *directorycheck;
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (pathnode->i_block[blockpointer] == 0){
            break;
        }
        lengthcomp = strlen(filename);
        directorycheck = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < 1024) {
            if(strncmp(filename, directorycheck->name, directorycheck->name_len) == 0 && lengthcomp == directorycheck->name_len) {
                printf("a file or directory with the name already exists\n");
                return EEXIST;
            } else {
                sizecheck += directorycheck->rec_len;
                directorycheck = (void *) directorycheck + directorycheck->rec_len;
            }
        }
    }
    
    
    int inode_bitmap[32];
    char* ibmap = (char *)(disk + 1024 * bg->bg_inode_bitmap);
    int i, pos;
    char temp;
    for (i = 0; i < 4; i+=1, ibmap +=1) {
        temp = *ibmap;
        for (pos = 0; pos < 8; pos+=1) {
            inode_bitmap[(8 * i) + pos] = (temp >> pos) & 1;
        }
    }
    
    
    int free_inode = -1;
    for (i = 0; i < 32; i+=1){
        if (inode_bitmap[i] == 0){
            free_inode = i + 1;
            inode_bitmap[i] = 1;
            break;
        }
    }
    if(free_inode == -1) {
        printf("no free inodes\n");
        return ENOSPC;
    }
    
    struct ext2_inode *newnode = itable + free_inode - 1;
    
    int block_bitmap[128];
    char* bbmap = (char *)(disk + 1024 * bg->bg_block_bitmap);
    for (i = 0; i < 16; i+=1, bbmap +=1) {
        temp = *bbmap;
        for (pos = 0; pos < 8; pos+=1) {
            block_bitmap[(8 * i) + pos] = (temp >> pos) & 1;
        }
    }

    //need a new block for our directory(blockneeded is just 1)
    int j;
    int m = 0;
    for (i = 0; i < 12 && i < blockneeded; i += 1){
        for (j = 0; j < 128; j +=1){
            if (block_bitmap[j] == 0) {
                block_bitmap[j] = 1;
                blockused +=1;
                newnode->i_block[i] = j + 1;
                break;
            }
        }
    }
    
    newnode->i_mode = EXT2_S_IFDIR | S_IRWXO;
    newnode->i_size = 1024;
    newnode->i_blocks = blockneeded * 2;
    newnode->i_links_count = 2;
    newnode->i_dtime = 0;
    struct ext2_dir_entry_2 *oldentry;
    struct ext2_dir_entry_2 *newentry;
    int spaceneeded = 8 + lengthcomp + (4 - lengthcomp % 4);
    int spaceold, oldsize, unusedblock;
    check = 0;
    //add our new directory entry to the parent directory
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (pathnode->i_block[blockpointer] == 0){
            unusedblock = blockpointer;
            break;
        }
        oldentry = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < 1024) {
            sizecheck += oldentry->rec_len;
            spaceold = 8 + oldentry->name_len + (4 - oldentry->name_len % 4);
            if (sizecheck == 1024 && oldentry->rec_len >= spaceneeded + spaceold){
                check = 1;
                oldsize = oldentry->rec_len;
                oldentry->rec_len = spaceold;
                newentry = (void *) oldentry + spaceold;
                newentry->inode = free_inode;
                newentry->rec_len = oldsize - spaceold;
                newentry->name_len = lengthcomp;
                newentry->file_type = 2;
                strncpy(newentry->name, filename, lengthcomp);
                break;
            } else {
                oldentry = (void *) oldentry + oldentry->rec_len;
            }
        }
        if (check == 1){
            break;
        }
    }
    //we hit a unused block so we use that instead of a used one
    if(check != 1) {
        for (m = 0; m < 128; m +=1){
             if (block_bitmap[m] == 0) {
                block_bitmap[m] = 1;
                blockused +=1;
                pathnode->i_block[unusedblock] = m + 1;
                pathnode->i_size += 1024;
                pathnode->i_blocks += 2;
                break;
            }
        }
        newentry = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[unusedblock]);
        newentry->inode = free_inode;
        newentry->rec_len = 1024;
        newentry->name_len = lengthcomp;
        newentry->file_type = 2;
        strncpy(newentry->name, filename, lengthcomp);
    }
    //need  . and .. entries for our new directory
    struct ext2_dir_entry_2 *selfentry;
    struct ext2_dir_entry_2 *parententry;
    char dot[1] = ".";
    char dotdot[2] = "..";
    selfentry = (struct ext2_dir_entry_2 *) (disk + 1024 * newnode->i_block[0]);
    selfentry->inode = free_inode;
    selfentry->rec_len = 12;
    selfentry->name_len = 1;
    selfentry->file_type = 2;
    strncpy(selfentry->name, dot, 1);
    
    parententry = (void *) selfentry + 12;
    parententry->inode = parentnode;
    parententry->rec_len = 1012;
    parententry->name_len = 2;
    parententry->file_type = 2;
    strncpy(parententry->name, dotdot, 2);
    //update our bitmap
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
    sb->s_free_blocks_count -= blockused;
    sb->s_free_inodes_count -= 1;
    return 0;
}
