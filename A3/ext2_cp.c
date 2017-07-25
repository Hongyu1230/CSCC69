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
	FILE *source = fopen(argv[2], "r");
	if (source == NULL) {
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
	char *token;
	char *token2;
	const char delimiter[2] = "/";
	char sourcename[strlen(sourcepath)];
	char destinationsplit[strlen(destpath)];
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
	
	fseek(source, 0L, SEEK_END);
	int sz = ftell(source);
	int blockneeded = ceil(sz/1024);
	int indirection_needed = 0;
	if (blockneeded > 12) {
		blockneeded += 1;
		indirection_needed = 1;
	}
	
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
	
	if (blockneeded > sb->s_free_blocks_count) {
		perror("not enough space for the new file");
		return ENOSPC;
	}
	
	struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
	struct ext2_inode *itable = (struct ext2_inode *)(disk + 1024 * bg->bg_inode_table);
	struct ext2_inode *pathnode = itable + 1;
	token2 = strtok(destpath, delimiter);
	int sizecheck, check = 0;
	struct ext2_dir_entry_2 *directory;
	while (token2 != NULL && S_ISDIR(pathnode->i_mode)) {
		directory = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[0]);
		while (sizecheck < pathnode->i_size) {
			if(strncmp(token2, directory->name, directory->name_len)) {
				pathnode = itable + directory->inode - 1;
				sizecheck = 0;
				check = 1;
				token2 = strtok(NULL, delimiter);
				break;
			} else {
				sizecheck += directory->rec_len;
				directory = (void *) directory + directory->rec_len;
			}
		}
		if (check == 1) {
			check = 0;
		} else {
			break;
		}
	}
	if (token2 != NULL) {
		//we couldn't reach the file destination, since we didn't go through all tokens
		perror("cannot find destination directory on disk");
		return ENOENT;
	}
	
	
	directory = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[0]);
	check = 0;
	sizecheck = 0;
	while (sizecheck < pathnode->i_size) {
		if(strncmp(sourcename, directory->name, directory->name_len) && directory->file_type == 1) {
			perror("the file at the location already exist");
			return EEXIST;
		}
		sizecheck += directory->rec_len;
		directory = (void *) directory + directory->rec_len;
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
			free_inode = i;
			break;
		}
	}
	if(free_inode == -1) {
		perror("no free inodes");
		return ENOSPC;
	}
	
	struct ext2_inode *newnode = itable + free_inode;
	
	int block_bitmap[128];
	char* bbmap = (char *)(disk + 1024 * bg->bg_block_bitmap);
    for (i = 0; i < 16; i+=1, bbmap +=1) {
        temp = *bbmap;
        for (pos = 0; pos < 8; pos+=1) {
            block_bitmap[(8 * i) + pos] = (temp >> pos) & 1;
        }
    }
	int o;
	for (i = 0; i < 16; i+=1) {
		for (o = 0; o <8; o +=1){
            printf("%d", block_bitmap[i*8 + o]);
		}
		printf("\n");
    }
	
	int j, k, l;
	char *mappos;
	for (i = 0; i < 12 && i < blockneeded; i += 1){
		for (j = 0; j < 128; j +=1){
			if (block_bitmap[j] == 0) {
				k = floor(j/8);
				l = 2^(j%8);
			}
		}
	}
	printf("%d\n%d", k, l);
    return 0;
}