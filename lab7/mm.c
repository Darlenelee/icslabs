/* Yijun Li 516030910395  ver 1.2
 * Implicit implementation
 * Next-fit strategy
 * Other words: Admittedly, ver 2.0 (explicitly organized and use double-way linked list)
 * was a failure due to some unfixed bugs in 3 traces, and I am not able to hand in that file for Lab7.
 * Still, I got inspired from all the three ways of implementation and got quite much practice in 
 * pointer manipulation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
#define ALIGNMENT 8
// MACROS
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8          
#define CHUNKSIZE (1<<12) 

#define MAX(x,y)    ((x)>(y)?(x):(y))

#define PACK(size,alloc)    ((size) | (alloc))

#define GET(p)  (*(unsigned int *)(p))
#define PUT(p,val)  (*(unsigned int *)(p) = (val))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)
#define HDRP(bp)    ((char *)(bp)-WSIZE)
#define FTRP(bp)    ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp)   ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))
static char *heap_listp = 0;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp,size_t asize);
static void *last;
/*
 * mm_init - initialize the malloc package.
 * The return value should be -1 if there was a problem, 0 otherwise
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){
        return -1;
    }
    PUT(heap_listp,0);
    PUT(heap_listp+(1*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp+(2*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp+(3*WSIZE),PACK(0,1));
    heap_listp += (2*WSIZE);
    last = heap_listp;
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }
    return 0;
}
static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words%2) ? (words+1)*WSIZE : words*WSIZE;
    if((long)(bp=mem_sbrk(size))==(void *)-1)
        return NULL;

    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));
    return coalesce(bp);
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;
    if(size ==0) return NULL;

    if(size <= DSIZE){
        asize = 2*(DSIZE);
    }else{
        asize = (DSIZE)*((size+(DSIZE)+(DSIZE-1)) / (DSIZE));
    }
    if((bp = find_fit(asize))!= NULL){
        place(bp,asize);
        return bp;
    }
    extendsize = MAX(asize,CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE))==NULL){
        return NULL;
    }
    place(bp,asize);
    return bp;
}

/*
 * mm_free - change the two marks of alloc
 */
void mm_free(void *bp)
{
    if(bp == 0)
	return;
    size_t size = GET_SIZE(HDRP(bp));
    // change ftr and hdr
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}
/*
 * coalesce - unite nearby blocks
 */
static void *coalesce(void *bp){
    size_t  prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t  next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) {     
        // CASE 1
        return bp;
    }
    else if(prev_alloc && !next_alloc){
        // CASE 2
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
    }
    else if(!prev_alloc && next_alloc){
        // CASE 3
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    else {
        // CASE 4
        size += GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    if ((last > (char *)bp) && (last < NEXT_BLKP(bp))) 
        last = bp;
    return bp;
}
/*
 * find_fit - Implemented with next-fit strategy, which shows greater throughput than first-fit
 */
static void *find_fit(size_t size){   
    char *old = last;

    /* Search from the last to the end of list */
    for ( ; GET_SIZE(HDRP(last)) > 0; last = NEXT_BLKP(last))
        if (!GET_ALLOC(HDRP(last)) && (size <= GET_SIZE(HDRP(last))))
            return last;

    /* search from start of list to old last */
    for (last = heap_listp; last < old; last = NEXT_BLKP(last))
        if (!GET_ALLOC(HDRP(last)) && (size <= GET_SIZE(HDRP(last))))
            return last;

    return NULL; 
}
/*
 * place - To place the block according to their size
 * bigger blocks will be splited 
 * smaller ones just be allocated as a whole
 */
static void place(void *bp,size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    if((csize-asize)>=(2*DSIZE)){
        // split into two blocks
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp),PACK(csize-asize,0));
        PUT(FTRP(bp),PACK(csize-asize,0));
    }else{
        // do not split
        PUT(HDRP(bp),PACK(csize,1));
        PUT(FTRP(bp),PACK(csize,1));
    }
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    /* If size == 0, the call is equivalent to mm_free(ptr) */
    if(size == 0) {
	    mm_free(ptr);
	    return 0;
    }

    /* If ptr is NULL, the call is equivalent to mm_malloc. */
    if(ptr == NULL) {
	    return mm_malloc(size);
    }
    newptr = mm_malloc(size);
    if(!newptr) {
	    return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);
    
    return newptr;
}

int mm_check(void){
    printf("===================\n");
    //print all the blocks
    for(void *bp = heap_listp; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
        printf("bp:%u\n",bp);
        printf("block size:%u\n",GET_SIZE(HDRP(bp)));
        printf("block allocated or not:%u\n",GET_ALLOC(HDRP(bp)));
    }

    return 0;
};