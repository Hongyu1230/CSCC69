#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

#include <string.h>

unsigned char *disk;


int main(int argc, char **argv) {


	if(argc != 3) {
		fprintf(stderr, "Usage: ext2_ls <image file name> <path on file disk>\n");
		exit(1);
	}

	
	int fd = open(argv[1], O_RDWR);
	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);

	}

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    printf("Block group:\n");

	printf("path is %s has length %lu\n", argv[2], strlen(argv[2]));
	//char path[] = argv[2];
	

	int i, test;
	
	
	char path[strlen(argv[2])];
	strcpy(path, argv[2]);

	char *list[strlen(argv[2])]; 

	char *segment = strtok(path, "/");

	
	for (i = 0; segment != NULL  ; i++) {

		list[i] = segment;
		segment = strtok (NULL, "/");
	}

	test = 0;
	while (test != i) {
		printf("%s\n", list[test]);
		test++;
	}

	//int depth = 0;

	/**
	int a = 0;
	int option;
	//flag raiser
	while ((option = getopt (argc, argv, "a")) != EOF)
		switch (option)	{
		
		case 'a':
			a++;

		}
	}


	**/
	


	//fd = open(argv[2], O_RDONLY);
	
	return 0;
	
}



