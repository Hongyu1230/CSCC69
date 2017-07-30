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
#include <time.h>
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {
	
	int r = 0;
    int option;
    while ((option = getopt (argc, argv, ":r")) != EOF) {
        switch (option) {
        case 'r':
            r++;
            break;
        default:
            break;

        }
    }

    if(argc != 3 + r) {
        fprintf(stderr, "Usage: ext2_rm_bonus <image file name> [-r] <FILE>\n");
        exit(1);
    }
    int fd = open(argv[1 + r], O_RDWR);
    char destpath[strlen(argv[2 + r]) + 1];
    char destpath2[strlen(argv[2 + r]) + 1];
    strcpy(destpath, argv[2 + r]);
    strcpy(destpath2, argv[2 + r]);
    if (destpath[0] != '/') {
        printf("the path needs to start from root, beginning with /\n");
        return ENOENT;
    }
    if (destpath[strlen(argv[2]) - 1] == '/') {
        printf("the removed file cannot end with /, needs to be a regular file not a directory\n");
        return EISDIR;
    }
    char *token;
    char *token2;
    const char delimiter[2] = "/";
    char filename[strlen(destpath) + 1];
    token = strtok(destpath, delimiter);
    int stoppoint = 0;
    //get the filename we are supposed to remove
    while (token != NULL) {
        strcpy(filename, token);
        stoppoint +=1;
        token = strtok(NULL, delimiter);
    }
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        printf("mmap");
        exit(1);
    }
    
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_inode *itable = (struct ext2_inode *)(disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *pathnode = itable + 1;
    token2 = strtok(destpath2, delimiter);
    int sizecheck, check = 0, blockpointer, found = 0, lengthcomp, startpoint = 0;
    struct ext2_dir_entry_2 *directory;
    //traversal to the parent directory of our path
    while (token2 != NULL && startpoint < stoppoint - 1) {
        startpoint +=1;
        lengthcomp = strlen(token2);
        for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
            if (pathnode->i_block[blockpointer] == 0){
                printf("cannot one of the files on the file path\n");
                return ENOENT;
            }
            directory = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
            sizecheck = 0;
            while (sizecheck < pathnode->i_size) {
                if(strncmp(token2, directory->name, directory->name_len) == 0 && lengthcomp == directory->name_len) {
                    pathnode = itable + directory->inode - 1;
                    if (!(S_ISDIR(pathnode->i_mode))) {
                        printf("one of the files on the path to the file is not a directory\n");
                        return ENOENT;
                    }
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
            printf("cannot one of the files on the file path\n");
            return ENOENT;
        }
    }
    
    check = 0;
    struct ext2_dir_entry_2 *directorycheck;
    struct ext2_inode *deletionnode;
    struct ext2_dir_entry_2 *deletiondirectory;
    //make sure the file at the location is not a directory if the r tag is not on
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
		if (pathnode->i_block[blockpointer] == 0){
            break;
        }
        lengthcomp = strlen(filename);
        directorycheck = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < pathnode->i_size) {
            if(strncmp(filename, directorycheck->name, directorycheck->name_len) == 0 && lengthcomp == directorycheck->name_len) {
                if (directorycheck->file_type == 2 && r == 0) {
                    printf("the file at the location is a directory\n");
                    return EISDIR;
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
            deletiondirectory = directorycheck;
            break;
        }
    }
    //we never found our target
    if (check == 0) {
        printf("cannot find the file at the location given\n");
        return ENOENT;
    }
	//if this is a file, we do nothing different, call our old command
	if (deletiondirectory->file_type != 2) {
		char command[strlen(argv[1 + r]) + strlen(argv[2 + r]) + 30];
		sprintf(command, "./ext2_rm %s %s", argv[1 + r], argv[2 + r]);
		system(command);
		return 0;
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
    deletionnode->i_links_count -= 1;
    unsigned int newdtime = (unsigned int) time(NULL);
    deletionnode->i_dtime = newdtime;
    int blockfreed = 0;
    //zero out the bitmap for inode and blocks for our operation later if we need it
    for (i = 0; i < 12; i +=1){
        if (deletionnode->i_block[i] == 0) {
            break;
        } else {
            blockfreed += 1;
            block_bitmap[deletionnode->i_block[i] - 1] = 0;
        }   
    }
    inode_bitmap[deletiondirectory->inode - 1] = 0;
    struct ext2_dir_entry_2 *oldentry;
    int oldsize, oldlen;
    check = 0;
	//recursively remove all in the deletion directory
	for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (deletionnode->i_block[blockpointer] == 0){
            break;
        }
        oldentry = (struct ext2_dir_entry_2 *)(disk + 1024 * deletionnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < 1024) {
			sizecheck += oldentry->rec_len;
			char oldname[255];
			strncpy(oldname, oldentry->name, oldentry->name_len);
			int oldtype = oldentry->file_type;
			oldentry = (void *) oldentry + oldentry->rec_len;
			char command[strlen(argv[1 + r]) + strlen(argv[2 + r]) + 300];
			if (oldtype == 2){
				sprintf(command, "./ext2_rm_bonus %s -r %s/%s", argv[1 + r], argv[2 + r], oldname);
			} else {
		        sprintf(command, "./ext2_rm %s %s/%s", argv[1 + r], argv[2 + r], oldname);
			}
		    system(command);
        }
        if (check == 1){
            break;
        }
    }
	
	
    //remove the entry in the parent directory
    for (blockpointer = 0; blockpointer < 12; blockpointer+=1) {
        if (pathnode->i_block[blockpointer] == 0){
            break;
        }
        oldentry = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[blockpointer]);
        sizecheck = 0;
        while (sizecheck < 1024) {
            if (oldentry == deletiondirectory) {
                check = 1;
                //we either add the rec_len to the previous block or zero it out and leave the rec_len
                if (sizecheck > 0) {
                    oldentry = (void *) oldentry - oldlen;
                    oldentry->rec_len += deletiondirectory->rec_len;
                    break;
                } else {
                    oldsize = deletiondirectory->rec_len;
                    memset(deletiondirectory, 0, deletiondirectory->rec_len);
                    deletiondirectory->rec_len = oldsize;
                    break;
                }
            } else {
                oldlen = oldentry->rec_len;
                oldentry = (void *) oldentry + oldentry->rec_len;
            }
            sizecheck += oldentry->rec_len;
        }
        if (check == 1){
            break;
        }
    }
    
    //if the inode has no links left, we can safely release the inode and blocks on the bitmap
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