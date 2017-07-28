#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {
	
	int s = 0;
	int option;
	while ((option = getopt (argc, argv, ":s")) != EOF) {
		switch (option)	{
		case 's':
			s++;
			break;
		default:
			break;

		}
	}

    if(argc != 4 + s) {
        fprintf(stderr, "Usage: ext2_ln <image file name> <LINK> <SOURCE>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);
    char sourcepath[strlen(argv[3 + s])];
    char destpath[strlen(argv[2 + s])];
	char sourcepath2[strlen(argv[3 + s])];
    char destpath2[strlen(argv[2 + s])];
    strcpy(sourcepath, argv[3 + s]);
    strcpy(destpath, argv[2 + s]);
	strcpy(sourcepath2, argv[3 + s]);
    strcpy(destpath2, argv[2 + s]);
    if (destpath[0] != '/' || sourcepath[0] != '/') {
        perror("the paths needs to start from root, beginning with /");
        return ENOENT;
    }
    char *token;
    char *token2;
	char *token3;
    char *token4;
    const char delimiter[2] = "/";
    char sourcename[strlen(sourcepath)];
	char destname[strlen(destpath)];
    char destinationsplit[strlen(destpath)];
    token = strtok(sourcepath, delimiter);
	token3 = strtok(destpath, delimiter);
	int sourcelength = 0, destlength = 0;
	
    return 0;
}