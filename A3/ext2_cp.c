#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
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
	char* dest;
	dest = argv[3];
	if(dest[0] != '/') {
		return ENOENT;
	}
	int destlen = strlen(dest)
	int i = destlen - 1;
	while (dest[i] != '/') {
		i-=1;
	}
	char sourcename[destlen];
	strncpy(sourcename, dest + i, destlen - 1);
	printf("%s",sourcename);
	
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
	
	
    return 0;
}