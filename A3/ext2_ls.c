#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

#include <string.h>
#include <errno.h>

unsigned char *disk;


int main(int argc, char **argv) {




	//flag raiser
	int a = 0;
	int option;
	while ((option = getopt (argc, argv, ":a")) != EOF) {
		switch (option)	{
		
		case 'a':
			printf("-a option\n");
			a++;
			break;
		
		default:
			break;

		}
	}

	if(argc != 3 + a) {
		fprintf(stderr, "Usage: ext2_ls <image file name> [a] <path on file disk>\n");
		exit(1);
	}
	

	//!!! argv[1] becomes "-a" after getopt for some reason
	
	int fd = open(argv[1+a], O_RDWR);
	printf("arg1 is %s\n", argv[1+a]);
	printf("arg2 is %s\n", argv[2+a]);
	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);

	}

	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

	printf("Inodes: %d\n", sb->s_inodes_count);
	printf("Blocks: %d\n", sb->s_blocks_count);
	printf("Block group:\n");


	int i, test;
	int depth = 0;
	
	char path[strlen(argv[2+a])];
	strcpy(path, argv[2+a]);

	char *list[strlen(argv[2+a])]; 

	char *segment = strtok(path, "/");

	printf("path is %s has length %lu\n", argv[2+a], strlen(argv[2+a]));
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

	
	int level = 1; //the depth level of branching inodes we're at
	int length; //keeps track cumulative directory length from start
	int invalid = 0; // 1 if any segment of path wrong,
	int discovered = 0;
	char name[100]; 
	
	struct ext2_dir_entry_2 *dir_entry;
	//struct ext2_dir_entry_2 *dir;
	printf("\nDirectory Blocks:\n");
	for (i = 1; i < 32 && level <= depth && invalid == 0; i+=1){
		
		discovered = 0; // set or reset for new inode
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
		invalid = 1; //won't switch off unless we find a matching level entry in enode

		while (length < inode->i_size){
			dir_entry = (struct ext2_dir_entry_2 *) (disk + ((1024 * (inode->i_block[0]))+length));
			//printf("Inode: %i rec_len: %d name_len: %i type= %i name=%s \n", dir_entry->inode, 
			//dir_entry->rec_len, dir_entry->name_len, dir_entry->file_type, dir_entry->name);

			length += dir_entry->rec_len;

			strncpy(name, dir_entry->name, dir_entry->name_len);
			printf("Inode %s at level %i. In depth %i of our arg, is this %s\n", name, level, depth, list[level-1]);
				
			//compares path segment at same level to entry name
			if (strcmp(list[level-1], name) == 0) {
				printf("  DING FOR %s\n", name);
				invalid = 0;
				if(level == depth){
					discovered = dir_entry->inode;
					printf("  discovered: %i\n", discovered);
					if (dir_entry->file_type == EXT2_FT_REG_FILE || dir_entry->file_type == EXT2_FT_SYMLINK){
						printf("DISCOVERED: \n");						
						printf("\nInode: %i rec_len: %d name_len: %i type= %i name=%.*s \n", 
						dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, 
						dir_entry->file_type, dir_entry->name_len, dir_entry->name);
					}
				}
			}	
			//wipeout name for use in other entries
			memset(name, 0, sizeof(name));
		}
		
		level++;
	}

	if (discovered != 0) {

		inode = (struct ext2_inode *) (inodeloc + sizeof(struct ext2_inode) * (discovered-1)); 
		if (!(inode->i_mode & EXT2_S_IFDIR)){
			//printf("Inode: %i rec_len: %d name_len: %i type= %i name=%s \n", dir_entry->inode, 
			//dir_entry->rec_len, dir_entry->name_len, dir_entry->file_type, dir_entry->name);
		}
		else if (inode->i_size == 0) {
			//printf("size zero\n"); Idk if needed
		}
		else {
			length = 0;
			printf("\nDISCOVERED:\n");
			//printf(" INODE SIZE: %d \n", inode->i_size);
			while (length < inode->i_size){
				dir_entry = (struct ext2_dir_entry_2 *) (disk + ((1024 * (inode->i_block[0]))+length));
				if (a == 1) {
					printf("Directory entry Inode: %i rec_len: %d name_len: %i type= %i name=%.*s \n", 
					dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, dir_entry->file_type, 
					dir_entry->name_len, dir_entry->name);
				}
				
				else if (strcmp(dir_entry->name, ".") != 0 && strcmp(dir_entry->name, "..") != 0) {
					printf("Directory entry Inode: %i rec_len: %d name_len: %i type= %i name=%.*s \n", 
					dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, dir_entry->file_type, 
					dir_entry->name_len, dir_entry->name);
				
				} 
				length += dir_entry->rec_len;
			}
		}
	}

	else {
		printf("\nNo such file or directory\n");
		return ENOENT;
	}
	
	return 0;
	
}



