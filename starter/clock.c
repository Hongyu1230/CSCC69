#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

static int clock;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	int flag = 0;
	int loop = 0;
	int i;
	int evicted;
	while (flag != 1) {
		//we went through a cycle already without a victim, reset clock to 0 and try again
		if (loop > 0) {
			clock = 0;
		}
		for (i=clock; i < memsize; i+=1) {
			if (coremap[i].pte->frame & PG_REF) {
				coremap[i].pte->frame &= ~PG_REF;
			} else {
				flag = 1;
				evicted = i;
				clock = i;
				break;
			}
		}
		loop+=1;
	}
	//move it to the next oldest one, since we got our evicted frame
	clock+=1;
	return evicted;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	p->frame |= PG_REF;
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clock = 0;
}
