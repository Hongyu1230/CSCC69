#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
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
		return ENOENT;
	}
	
	char sourcepath[strlen(argv[2])];
	char destpath[strlen(argv[3])];
	strcpy(sourcepath, argv[2]);
	strcpy(destpath, argv[3]);
	if (destpath[0] != '/') {
		return ENOENT;
	}
	int i;
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

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
	struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
	struct ext2_inode *itable = (struct ext2_inode *)(disk + 1024 * bg->bg_inode_table);
	struct ext2_inode *pathnode = itable + 1;
	token2 = strtok(destpath, delimiter);
	int sizecheck, check = 0;
	struct ext2_dir_entry_2 *directory;
	while (token2 != NULL && S_ISDIR(pathnode->i_mode)) {
		directory = (struct ext2_dir_entry_2 *)(disk + 1024 * pathnode->i_block[0]);
		while (sizecheck < pathnode->i_size) {
			if(strcmp(token2, directory->name)) {
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
		return ENOENT;
	}
    return 0;
}