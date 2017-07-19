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
    
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
    printf("block bitmap: %d\n", bg->bg_block_bitmap);
    printf("inode bitmap: %d\n", bg-> bg_inode_bitmap);
    printf("inode table: %d\n", bg->bg_inode_table);
    printf("free blocks: %d\n", bg->bg_free_blocks_count);
    printf("free inodes: %d\n", bg->bg_free_inodes_count);
    printf("used_dirs: %d\n", bg->bg_used_dirs_count);
    
    char* bbmap = (char *)(disk + 1024 * bg->bg_block_bitmap);
    printf("Block bitmap:");
    int i, pos;
    char temp;
    for (i = 0; i < 16; i+=1, bbmap +=1) {
        temp = *bbmap;
        for (pos = 0; pos < 8; pos++) {
            printf("%d", (temp >> pos) & 1);
        }
        printf(" ");
    }
    printf("\n");
    
    char* ibmap = (char *)(disk + 1024 * bg->bg_inode_bitmap);
    printf("Inode bitmap:");
    int j, pos2;
    char temp2;
    for (i = 0; i < 4; i+=1, ibmap +=1) {
        temp2 = *ibmap;
        for (pos = 0; pos < 8; pos++) {
            printf("%d", (temp2 >> pos) & 1);
        }
        printf(" ");
    }
    printf("\n");
    char* inodeloc = (char*)(disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *inode;
    char type = '0';
    printf("Inodes:\n");
    for (i = 1; i < 32; i+=1){
        inode = (struct ext2_inode *) (inodeloc + sizeof(struct ext2_inode) * i);
        if (inode->i_mode & EXT2_S_IFREG) {
            type = 'f';
        } else if (inode->i_mode & EXT2_S_IFDIR) {
            type = 'd';
        }
        if (inode->i_size == 0) {
            continue;
        }
        if (i < 11 && i != 1) {
            continue;
        }
        printf("[%d] type: %c size: %d links: %d blocks: %d\n", i + 1, type, inode->i_size, inode->i_links_count, inode->i_blocks);
        for (j=0; j < 15; j+=1) {
            printf("[%d] Blocks:  %d\n", i + 1, inode->i_blocks[j]);
        }
    }
    return 0;
}