#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

static int counter;

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
	//since we know how frames are assigned, which basically assigns it by the first unused frame
	//we know that the frame will be inputted from 0,1,2 ... memsize - 1
	//so we can just use a counter to choose our eviction considering this will only be called when the memory is full
	int evicted = counter;
	
	//increment counter, if our counter is at the end we need to reset it back to 0
    if (counter == (memsize - 1)) {
		counter = 0;
	}
	else {
		counter += 1;
	}

	return evicted;

}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {

	return;
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void fifo_init() {
	counter = 0;
}
