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
	for (i = strlen(sourcepath) - 1; i > 0; i-=1){
		if (sourcepath[i] == '/') {
			break;
		}
	}
	char sourcename[11];
	strncpy(sourcename, &sourcepath[i + 1], strlen(sourcepath) - 1 - i);
	printf("%s", sourcename);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
	
	
    return 0;
}