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

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
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
	
	struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
    printf("block bitmap: %d\n", bg->bg_block_bitmap);
    printf("inode bitmap: %d\n", bg-> bg_inode_bitmap);
    printf("inode table: %d\n", bg->bg_inode_table);
	printf("free blocks: %d\n", bg->bg_free_blocks_count);
	printf("free inodes: %d\n", bg->bg_free_inodes_count);
	printf("used_dirs: %d\n", bg->bg_used_dirs_count);
	
	char* bbmap = (char *)(disk + EXT2_BLOCK_SIZE * bg->bg_block_bitmap);
	printf("Block bitmap:");
	int i, pos;
	char temp;
	char bitmapv[8];
	for (i = 0; i < sb->s_blocks_count / 8; i+=1, bbmap +=1) {
		temp = *bbmap;
		for (pos = 0; pos < 8; pos++) {
			bitmapv[pos] = (temp >> pos) & 1;
		}
		printf("%s", bitmapv);
		printf(" ");
	}
	printf("\n");
	
    return 0;
}
