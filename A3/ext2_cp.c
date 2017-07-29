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

    if(argc != 4) {
        fprintf(stderr, "Usage: ext2_cp <image file name> <SOURCE> <DEST>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);
    int source = open(argv[2], O_RDONLY);
    if (source < 0) {
        perror("could not find source file");
        return ENOENT;
    }
    char sourcepath[strlen(argv[2])];
    char destpath[strlen(argv[3])];
    strcpy(sourcepath, argv[2]);
    strcpy(destpath, argv[3]);
    if (destpath[0] != '/') {
        perror("the path needs to start from root, beginning with /");
        return ENOENT;
    }
	if (sourcepath[strlen(argv[2]) - 1] == '/') {
        perror("the source cannot end with /, needs to be a regular file");
        return EISDIR;
    }
    char *token;
    char *token2;
    const char delimiter[2] = "/";
    char sourcename[strlen(sourcepath)];
    token = strtok(sourcepath, delimiter);
    while (token != NULL) {
        strcpy(sourcename, token);
        token = strtok(NULL, delimiter);
    }
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    float filesize = lseek(source, 0, SEEK_END);
    int blockneeded = ceil(filesize/1024);
    if (blockneeded > 12) {
        blockneeded += 1;
    }
	
    char *src;
    src = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, source, 0);
    
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    int blockused = 0;
	//want to make sure we have 1 extra block in case we need one for inserting a file/directory
    if (blockneeded + 1 > sb->s_free_blocks_count) {
        perror("not enough space for the new file");
        return ENOSPC;
    }
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_inode *itable = (struct ext2_inode *)(disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *pathnode = itable + 1;
    token2 = strtok(destpath, delimiter);
    int sizecheck, check = 0, blockpointer, found = 0, lengthcomp = 0;
    struct ext2_dir_entry_2 *directory;
    while (token2 != NULL && S_ISDIR(pathnode->i_mode)) {
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
            perror("cannot find destination directory on disk");
            return ENOENT;
        }
    }
    struct ext2_dir_entry_2 *directorycheck;
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (pathnode->i_block[blockpointer] == 0){
            break;
        }
        lengthcomp = strlen(sourcename);
        directorycheck = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < pathnode->i_size) {
            if(strncmp(sourcename, directorycheck->name, directorycheck->name_len) == 0 && lengthcomp == directorycheck->name_len) {
                perror("a file or directory with the name already exists");
                return EEXIST;
            } else {
                if (directorycheck->rec_len == 0) {
                    break;
                }
                sizecheck += directorycheck->rec_len;
                directorycheck = (void *) directorycheck + directorycheck->rec_len;
            }
        }
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
    
    
    int free_inode = -1;
    for (i = 0; i < 32; i+=1){
        if (inode_bitmap[i] == 0){
            inode_bitmap[i] = 1;
            free_inode = i + 1;
            break;
        }
    }
    if(free_inode == -1) {
        perror("no free inodes");
        return ENOSPC;
    }
    
    struct ext2_inode *newnode = itable + free_inode - 1;
    
    int block_bitmap[128];
    char *bbmap = (char *)(disk + 1024 * bg->bg_block_bitmap);
    for (i = 0; i < 16; i+=1, bbmap +=1) {
        temp = *bbmap;
        for (pos = 0; pos < 8; pos+=1) {
            block_bitmap[(8 * i) + pos] = (temp >> pos) & 1;
        }
    }
    
    int j, l, k;
    int m = 0;
    char *mappos;
    for (i = 0; i < 12 && i < blockneeded; i += 1){
        for (j = 0; j < 128; j +=1){
            if (block_bitmap[j] == 0) {
                block_bitmap[j] = 1;
                blockused +=1;
                newnode->i_block[i] = j + 1;
                memcpy(disk + 1024 * (j + 1), src + 1024*i, 1024);
                break;
            }
        }
    }
    int n;
    int *indirectionblock;
    if (blockneeded > 12) {
        //find a free block for our indirect
        for (j = 0; j < 128; j +=1){
            if (block_bitmap[j] == 0) {
                block_bitmap[j] = 1;
                blockused +=1;
                newnode->i_block[12] = j + 1;
                break;
            }
        }
        indirectionblock = (void *) (disk + 1024 * newnode->i_block[12]);
        for (i = 0; i < blockneeded - 12; i+=1){
            for (m = 0; m < 128; m +=1){
                if (block_bitmap[m] == 0) {
                    block_bitmap[m] = 1;
                    blockused +=1;
                    indirectionblock[i] = m + 1;
                    break;
                }
            }
        }
        for (n = 0; n < blockneeded - 12; n+=1) {
            memcpy(disk + 1024 * indirectionblock[n], src + 1024*(n+12), 1024);
        }
    }
    newnode->i_mode = EXT2_S_IFREG | S_IRWXO;
    newnode->i_size = filesize;
    newnode->i_blocks = blockneeded * 2;
    newnode->i_links_count = 1;
    newnode->i_dtime = 0;
    struct ext2_dir_entry_2 *oldentry;
    struct ext2_dir_entry_2 *newentry;
    int spaceneeded = 8 + lengthcomp + (4 - lengthcomp % 4);
    int spaceold, oldsize, unusedblock;
    check = 0;
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
                newentry->file_type = 1;
                strncpy(newentry->name, sourcename, lengthcomp);
                break;
            } else {
                oldentry = (void *) oldentry + oldentry->rec_len;
            }
        }
        if (check == 1){
            break;
        }
    }
    
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
        newentry->file_type = 1;
        strncpy(newentry->name, sourcename, lengthcomp);
    }
    
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