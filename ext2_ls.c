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
	int depth = 0;
	
	char path[strlen(argv[2])];
	strcpy(path, argv[2]);

	char *list[strlen(argv[2])]; 

	char *segment = strtok(path, "/");

	
	for (i = 0; segment != NULL  ; i++) {

		list[i] = segment;
		segment = strtok (NULL, "/");
		depth ++;
	}

	test = 0;
	while (test != i) {
		printf("%s\n", list[test]);
		test++;
	}


	//structures necessary for directory traversal
	struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
	char* inodeloc = (char*)(disk + 1024 * gd->bg_inode_table);
	struct ext2_inode *inode;

	
	int match = 1;
	int length;
	int discovered = 0;
	char name[100]; //make sizeof
	
	struct ext2_dir_entry_2 *dir_entry;
	//struct ext2_dir_entry_2 *dir;
	printf("\nDirectory Blocks:\n");
	for (i = 1; i < 32 && match <= depth; i+=1){
		inode = (struct ext2_inode *) (inodeloc + sizeof(struct ext2_inode) * i); 
		if (!(inode->i_mode & EXT2_S_IFDIR)){
			continue;
		}
		if (inode->i_size == 0) {
			continue;
		}
		if (i < 11 && i != 1) {
			continue;
		}

		
		
		printf("	DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[0], i + 1);
		
		length = 0;

		
		//printf(" INODE SIZE: %d \n", inode->i_size);
		while (length < inode->i_size){


			dir_entry = (struct ext2_dir_entry_2 *) (disk + ((1024 * (inode->i_block[0]))+length));
			//printf("Inode: %i rec_len: %d name_len: %i type= %i name=%s \n", dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, dir_entry->file_type, dir_entry->name);
			length += dir_entry->rec_len;
			//printf("Length: %d", length);
			
			
			
			//if (match <= depth) {
				strncpy(name, dir_entry->name, dir_entry->name_len);
				printf("name shortened %s level %i and depth %i compare %s\n", name, match, depth, list[match-1]);
				

				if (strcmp(list[match-1], name) == 0) {
					printf("DING FOR %s\n", name);
					discovered = dir_entry->inode;
				}
				//wipeout name for use
				memset(name, 0, sizeof(name));
			//}

						
			;
		}
		match++;
		

		
	}

	if (discovered != 0) {

		inode = (struct ext2_inode *) (inodeloc + sizeof(struct ext2_inode) * (discovered-1)); 
		if (!(inode->i_mode & EXT2_S_IFDIR)){
			printf("NOT VALID\n");
		}
		else if (inode->i_size == 0) {
			printf("NOT VALID\n");
		}
		else if (i < 11 && i != 1) {
			printf("NOT VALID\n");
		}

		
		else {
			length = 0;

			printf("DISCOVERED:\n");
			//printf(" INODE SIZE: %d \n", inode->i_size);
			while (length < inode->i_size){
				dir_entry = (struct ext2_dir_entry_2 *) (disk + ((1024 * (inode->i_block[0]))+length));
				printf("In directoryInode: %i rec_len: %d name_len: %i type= %i name=%.*s \n", dir_entry->inode, 
				dir_entry->rec_len, dir_entry->name_len, dir_entry->file_type, dir_entry->name_len, dir_entry->name);
				length += dir_entry->rec_len;
			}
		}
	}
	
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



