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
    int evicted = 0;
    //we run until flag is 1, that is we found a frame in memory without the ref bit on
    while (flag != 1) {
        //we went through a cycle already without a victim, reset clock to 0 and try again
        if (loop > 0) {
            clock = 0;
        }
        //turn off the ref bit if the ref bit is on, else we find one without the ref bit and decides our evicted frame
        //we set the clock to the found frame too, because we just found the frame to evict
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
    //move by one since we just got our evicted frame
    if (clock == (memsize - 1)) {
        clock = 0;
    }
    else {
        clock += 1;
    }
    return evicted;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
    return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
    clock = 0;
}
