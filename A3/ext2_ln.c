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
    
    int s = 0;
    int option;
    while ((option = getopt (argc, argv, ":s")) != EOF) {
        switch (option) {
        case 's':
            s++;
            break;
        default:
            break;

        }
    }

    if(argc != 4 + s) {
        fprintf(stderr, "Usage: ext2_ln <image file name> [-s] <SOURCE> <LINK>\n");
        exit(1);
    }
    int fd = open(argv[1 + s], O_RDWR);
    //need multiple because strtok breaks our string
    char sourcepath[strlen(argv[2 + s]) + 1];
    char destpath[strlen(argv[3 + s]) + 1];
    char sourcepath2[strlen(argv[2 + s]) + 1];
    char destpath2[strlen(argv[3 + s]) + 1];
    char sourcepath3[strlen(argv[2 + s]) + 1];
    strcpy(sourcepath, argv[2 + s]);
    strcpy(destpath, argv[3 + s]);
    strcpy(sourcepath2, argv[2 + s]);
    strcpy(destpath2, argv[3 + s]);
    strcpy(sourcepath3, argv[2 + s]);
    if (destpath[0] != '/' || sourcepath[0] != '/') {
        printf("the paths needs to start from root, beginning with /\n");
        return ENOENT;
    }
    if (destpath[strlen(argv[3 + s]) - 1] == '/') {
        printf("the destination path cannot end with a /\n");
        return EISDIR;
    }
    if (sourcepath[strlen(argv[2 + s]) - 1] == '/' && s == 0) {
        printf("the source cannot end with a /, needs to be a directory unless you are creating a symbolic link\n");
        return EISDIR;
    }
    char *token;
    char *token2;
    char *token3;
    char *token4;
    const char delimiter[2] = "/";
    char sourcename[strlen(sourcepath) + 1];
    char destname[strlen(destpath) + 1];
    token = strtok(sourcepath, delimiter);
    int sourcelength = 0, destlength = 0;
    //to get the source name of the link
    while (token != NULL) {
        strcpy(sourcename, token);
        sourcelength += 1;
        token = strtok(NULL, delimiter);
    }
    token3 = strtok(destpath, delimiter);
    //to get the destination name of the link
    while (token3 != NULL) {
        strcpy(destname, token3);
        destlength += 1;
        token3 = strtok(NULL, delimiter);
    }
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        printf("mmap");
        exit(1);
    }
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    int blockused = 0;
    int blockneeded;
    if (s == 0){
        blockneeded = 0;
    } else {
        float sizeforlink = strlen(destpath);
        blockneeded = ceil(sizeforlink/1024);
    }
    if (blockneeded + 1 > sb->s_free_blocks_count) {
        printf("not enough space for the new file\n");
        return ENOSPC;
    }
    
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_inode *itable = (struct ext2_inode *)(disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *pathnode = itable + 1;
    token2 = strtok(destpath2, delimiter);
    int sizecheck, check = 0, blockpointer, found = 0, lengthcomp = 0, startingpoint = 0;
    struct ext2_dir_entry_2 *directory;
    //traverse for our parent of the destination
    while (token2 != NULL && startingpoint < destlength - 1) {
        lengthcomp = strlen(token2);
        startingpoint += 1;
        for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
            if (pathnode->i_block[blockpointer] == 0){
                printf("cannot one of the files on the destination path\n");
                return ENOENT;
            }
            directory = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
            sizecheck = 0;
            while (sizecheck < pathnode->i_size) {
                if(strncmp(token2, directory->name, directory->name_len) == 0 && lengthcomp == directory->name_len) {
                    pathnode = itable + directory->inode - 1;
                    if (!(S_ISDIR(pathnode->i_mode))) {
                        printf("one of the files on the path to the destination is not a directory\n");
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
            printf("cannot find one of the paths for the destination location\n");
            return ENOENT;
        }
    }
    
    //find that we don't have a existing file for our link
    struct ext2_dir_entry_2 *directorycheck;
    unsigned int linknode;
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (pathnode->i_block[blockpointer] == 0){
            break;
        }
        lengthcomp = strlen(destname);
        directorycheck = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < pathnode->i_size) {
            if(strncmp(destname, directorycheck->name, directorycheck->name_len) == 0 && lengthcomp == directorycheck->name_len) {
                if (directorycheck->file_type == 2) {
                    printf("the destination path is already a directory\n");
                    return EISDIR;
                } else {
                    printf("the destination already has a file named that\n");
                    return EEXIST;
                }
            } else {
                sizecheck += directorycheck->rec_len;
                directorycheck = (void *) directorycheck + directorycheck->rec_len;
            }
        }
    }
    
    //rerun this to find our source for the link
    struct ext2_inode *destinationnode = pathnode;
    pathnode = itable + 1;
    token4 = strtok(sourcepath2, delimiter);
    check = 0;
    found = 0;
    int lengthcomps = 0;
    startingpoint = 0;
    while (token4 != NULL && startingpoint < sourcelength - 1) {
        lengthcomps = strlen(token4);
        startingpoint += 1;
        for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
            if (pathnode->i_block[blockpointer] == 0){
                printf("cannot one of the files on the source path\n");
                return ENOENT;
            }
            directory = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
            sizecheck = 0;
            while (sizecheck < pathnode->i_size) {
                if(strncmp(token4, directory->name, directory->name_len) == 0 && lengthcomps == directory->name_len) {
                    pathnode = itable + directory->inode - 1;
                    if (!(S_ISDIR(pathnode->i_mode))) {
                        printf("one of the files on the path to the source file is not a directory\n");
                        return ENOENT;
                    }
                    sizecheck = 0;
                    check = 1;
                    found = 1;
                    token4 = strtok(NULL, delimiter);
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
            printf("cannot find one of the paths for the link source\n");
            return ENOENT;
        }
    }
    
    check = 0;
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (pathnode->i_block[blockpointer] == 0){
            break;
        }
        lengthcomps = strlen(sourcename);
        directorycheck = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < pathnode->i_size) {
            if(strncmp(sourcename, directorycheck->name, directorycheck->name_len) == 0 && lengthcomps == directorycheck->name_len) {
                if (directorycheck->file_type == 2 && s == 0) {
                    printf("the source file is a directory\n");
                    return EISDIR;
                } else {
                    check = 1;
                    break;
                }
            } else {
                sizecheck += directorycheck->rec_len;
                directorycheck = (void *) directorycheck + directorycheck->rec_len;
            }
        }
        if(check == 1) {
            linknode = directorycheck->inode;
            break;
        }
    }
    
    if (check == 0) {
        printf("cannot find the source file to link\n");
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
    
    int free_inode = -1;
    struct ext2_inode *newnode;
    //we need a free inode if we are to do a symbolic link
    if (s != 0) {
        for (i = 0; i < 32; i+=1){
            if (inode_bitmap[i] == 0){
                inode_bitmap[i] = 1;
                free_inode = i + 1;
                break;
            }
        }
        if(free_inode == -1) {
            printf("no free inodes\n");
            return ENOSPC;
        }
        newnode = itable + free_inode - 1;
        newnode->i_mode = EXT2_S_IFLNK | S_IRWXO;
        newnode->i_size = strlen(sourcepath3);
        newnode->i_blocks = blockneeded * 2;
        newnode->i_links_count = 1;
        newnode->i_dtime = 0;
        int j;
        //path shouldn't exceed 4098, copy the path into the free blocks
        for (i = 0; i < 4 && i < blockneeded; i += 1){
            for (j = 0; j < 128; j +=1){
                if (block_bitmap[j] == 0) {
                    block_bitmap[j] = 1;
                    blockused +=1;
                    newnode->i_block[i] = j + 1;
                    memcpy(disk + 1024 * (j + 1), sourcepath3 + 1024*i, 1024);
                    break;
                }
            }
        }
    }
    
    struct ext2_inode *linkinode = itable + linknode - 1;
    if (s == 0) {
        linkinode->i_links_count += 1;
    }
    struct ext2_dir_entry_2 *oldentry;
    struct ext2_dir_entry_2 *newentry;
    int spaceneeded = 8 + lengthcomp + (4 - lengthcomp % 4);
    int spaceold, oldsize, unusedblock;
    check = 0;
    //adding the link to our parent directory for the link
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (destinationnode->i_block[blockpointer] == 0){
            unusedblock = blockpointer;
            break;
        }
        oldentry = (struct ext2_dir_entry_2 *)(disk + 1024 * destinationnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < 1024) {
            sizecheck += oldentry->rec_len;
            spaceold = 8 + oldentry->name_len + (4 - oldentry->name_len % 4);
            if (sizecheck == 1024 && oldentry->rec_len >= spaceneeded + spaceold){
                check = 1;
                oldsize = oldentry->rec_len;
                oldentry->rec_len = spaceold;
                newentry = (void *) oldentry + spaceold;
                if (s == 0) {
                    newentry->inode = linknode;
                    newentry->file_type = 1;
                } else {
                    newentry->inode = free_inode;
                    newentry->file_type = 7;
                }
                newentry->rec_len = oldsize - spaceold;
                newentry->name_len = lengthcomp;
                strncpy(newentry->name, destname, lengthcomp);
                break;
            } else {
                oldentry = (void *) oldentry + oldentry->rec_len;
            }
        }
        if (check == 1){
            break;
        }
    }
    
    int m;
    //we found a unusedblock, add our link to that instead
    if(check != 1) {
        for (m = 0; m < 128; m +=1){
             if (block_bitmap[m] == 0) {
                block_bitmap[m] = 1;
                blockused +=1;
                linkinode->i_block[unusedblock] = m + 1;
                linkinode->i_size += 1024;
                linkinode->i_blocks += 2;
                break;
            }
        }
        newentry = (struct ext2_dir_entry_2 *)(disk + 1024 * linkinode->i_block[unusedblock]);
        newentry->rec_len = 1024;
        newentry->name_len = lengthcomp;
        if (s == 0) {
            newentry->file_type = 1;
            newentry->inode = linknode;
        } else {
            newentry->file_type = 7;
            newentry->inode = free_inode;
        }
        strncpy(newentry->name, destname, lengthcomp);
    }
    //update our bitmaps
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
    if (s != 0) {
        sb->s_free_inodes_count -= 1;
    }

    return 0;
}