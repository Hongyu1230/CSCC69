#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#define MAXLINE 256


extern int memsize;

extern int debug;

extern struct frame *coremap;

extern char *tracefile;

static addr_t *addresslist;

static int line;

static int filesize;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	int evicted = 0;
	int i,o;
	int longest = 0;
	int sight = 0;
	for (i = 0; i < memsize; i += 1) {
		for (o = line + 1; o <= filesize && sight <= 30; o += 1) {
			sight += 1;
			if (coremap[i].pte->virtualaddress == addresslist[o] && o - line >= longest && coremap[i].pte->checked != 1){
				longest = o - line;
				evicted = i;
				coremap[i].pte->checked = 1;
			}
		}
	}
	for (i = 0; i < memsize; i += 1) {
		if (coremap[i].pte->checked != 1) {
			evicted = i;
		}
	}
	
	return evicted;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	p->virtualaddress = addresslist[line];
	line +=1;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	char buf[MAXLINE];
	addr_t vaddr = 0;
	char type;
	FILE *tfp;
	int i = 0;
	if(tracefile != NULL) {
		if((tfp = fopen(tracefile, "r")) == NULL) {
			perror("Error opening tracefile:");
			exit(1);
		}
	}
	
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			filesize+=1;
		} else {
			continue;
		}
	}
	addresslist = malloc(sizeof(addr_t) * filesize);
	rewind(tfp);
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			addresslist[i] = vaddr;
			i += 1;
		} else {
			continue;
		}
	}
	
	line = 0;
}

