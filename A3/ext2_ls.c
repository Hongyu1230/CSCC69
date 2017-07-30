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
	//! argv[1] becomes "-a" after getopt for some reason
	int fd = open(argv[1+a], O_RDWR);
	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	//parses path argument
	int i, store;
	int depth = 0;
	char path[strlen(argv[2+a])+1];
	strcpy(path, argv[2+a]);
	char *list[strlen(argv[2+a])+1]; 
	char *segment = strtok(path, "/");

	for (i = 0; segment != NULL  ; i++) {
		list[i] = segment;
		segment = strtok (NULL, "/");
		depth ++;
	}
   	if (path[0] != '/') {
        	perror("the path needs to start from root, beginning with /");
    		return ENOENT;
    	}

	//structures necessary for directory traversal
	struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
	char* inodeloc = (char*)(disk + 1024 * gd->bg_inode_table);
	struct ext2_dir_entry_2 *dir_entry;
	struct ext2_inode *itable = (struct ext2_inode *) (inodeloc);
	struct ext2_inode *pathnode = itable + 1;
	
	int level = 1; //the depth level of branching inodes we're at
	int length; //keeps track cumulative directory length from start
	int invalid = 0; // 1 if any segment of path wrong
	int lvl_clear; //1 if matching segment found
	int discovered = 0;

	while (level <= depth && S_ISDIR(pathnode->i_mode)) {
		//set for new path segment
		lvl_clear = 0;

		//for every inode the i_block is pointing to
		//conditions: valid previous segment, segment not found yet, pointer is pointing a nonzero inode
		for (i = 0; i < 12  && invalid == 0 && lvl_clear == 0 && pathnode->i_block[i] != 0; i+=1){
			length = 0;
			invalid = 1; //won't switch off unless we find a matching level entry in enode

			//while we are still within the node's boundaries and haven't found a segment
			while (length < pathnode->i_size && lvl_clear == 0){
				dir_entry = (struct ext2_dir_entry_2 *) (disk + ((1024 * (pathnode->i_block[i]))+length));
				length += dir_entry->rec_len;
				
				//compares path segment at same level to entry name
				if (strncmp(list[level-1], dir_entry->name, dir_entry->name_len) == 0 
				&& strlen(list[level-1]) == dir_entry->name_len) {
					invalid = 0;
					lvl_clear = 1;
					//move into the matching (directory) inode
					pathnode = itable + dir_entry->inode - 1;

					//check if next segment is the null, indicating final segment in path
					if(level == depth){
						discovered = dir_entry->inode;
						//store the pointer block the inode is in
						store = i;

						//if it is just a regular file or symlink, print it's name on the spot
						if (dir_entry->file_type == EXT2_FT_SYMLINK || 
						dir_entry->file_type == EXT2_FT_REG_FILE){					
							printf("%.*s\n", dir_entry->name_len, dir_entry->name);						
						}	
					}
					else if (dir_entry->file_type != EXT2_FT_DIR) {
						printf("One of the files on the path to the file is not a directory\n");
						return ENOENT;
					}

									
				}	
				else {
					if (dir_entry->rec_len == 0) {		
						break;
					}
				}
			}
		}
		level++;
	}

	//if root directory, we need to go into 2nd inode
	if (discovered == 0 && invalid == 0) { discovered = 2; }

	//we want to now print the contents of the final directory in path, if found
	if (discovered != 0) {
		pathnode = (struct ext2_inode *) (inodeloc + sizeof(struct ext2_inode) * (discovered-1)); 

		//pathnode = itable + discovered-1
		if (S_ISDIR(pathnode->i_mode)) {
			length = 0;

			//print all contents for directory inode
			while (length < pathnode->i_size){	
				dir_entry = (struct ext2_dir_entry_2 *) (disk + ((1024 * (pathnode->i_block[store]))+length));

				if (a == 1) {
					printf("%.*s\n", dir_entry->name_len, dir_entry->name);
				}
				
				//print . and .. entries if a flag specified
				else if (strcmp(dir_entry->name, ".") != 0 && strcmp(dir_entry->name, "..") != 0) {
					printf("%.*s\n", dir_entry->name_len, dir_entry->name);		
				} 

				length += dir_entry->rec_len;
			}
		}
	}

	else {
		printf("No such file or directory\n");
		return ENOENT;
	}
	
	
	return 0;
	
}




