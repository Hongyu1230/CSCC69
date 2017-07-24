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
	FILE *source = fopen(argv[2], "r");
	if (source== NULL) {
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
	char* token,token2;
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

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
	struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
	struct ext2_inode *itable = (struct ext2_inode *)(disk + 1024 * bg->bg_inode_table);
	struct ext2_inode *inode = itable + 1;
	char destinationsplit[strlen(destpath)];
	token2 = strtok(destpath, delimiter);
	while (token2 != NULL) {
		strcpy(destinationsplit, token);
		printf("%s", destinationsplit);
		token2 = strtok(NULL, delimiter);
	}
    return 0;
}