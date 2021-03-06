// Yijun Li 516030910395
// use explicit list to organize free blocks
// use LIFO strategy to alloc new blocks 
// the recently freed block pointer will be stored in head
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"
// MACROS
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define WSIZE 4
#define DSIZE 8          
#define CHUNKSIZE (1<<12) 
#define MAX(x,y)    ((x)>(y)?(x):(y))
#define PACK(size,alloc)    ((size) | (alloc))
#define GET(p)  (*(unsigned int *)(p))
#define GET_8(p)  (*(long *)(p))
#define PUT(p,val)  (*(unsigned int *)(p) = (val))
#define PUT_8(p,val)  (*(long *)(p) = (val))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)     (GET(p) & 0x1)
#define HDRP(bp)    ((char *)(bp)-WSIZE)
#define FTRP(bp)    ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp)   ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

#define GET_PRED(bp) (GET_8((char *)(bp)))
#define GET_SUCC(bp) (GET_8((char *)(bp)+DSIZE))
#define SET_PRED(bp, val) (PUT_8((char *)(bp), (val)))
#define SET_SUCC(bp, val) (PUT_8(((char *)(bp)+DSIZE), (val)))
static char *heap_listp = 0;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp,size_t asize);
static void *insert(void *bp);
int mm_check(char func[]);
static void *head = NULL;  // top of the stack

/*
 * mm_init - initialize the malloc package.
 * The return value should be -1 if there was a problem, 0 otherwise
 */
int mm_init(void)
{
	head = NULL;
    //printf("in mm_init\n");//just test
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){
        return -1;
    }
    PUT(heap_listp,0);
    PUT(heap_listp+(1*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp+(2*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp+(3*WSIZE),PACK(0,1));
    heap_listp += (2*WSIZE);
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }
    
    return 0;
}
static void *extend_heap(size_t words){
    //printf("in extend_heap\n");//just test
    char *bp;
    size_t size;

    size = (words%2) ? (words+1)*WSIZE : words*WSIZE;
    if((long)(bp = mem_sbrk(size)) == ((void *)-1))
        return NULL;

    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
	SET_SUCC(bp,0);
	SET_PRED(bp,0);
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));
    
    return coalesce(bp);
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    //printf("in malloc\n");//just test
    size_t asize;
    size_t extendsize;
    char *bp;
    if(size ==0) return NULL;

    if(size <= DSIZE){
        asize = 3*(DSIZE);
    }
    else{
        asize = (DSIZE)*((size+2*(DSIZE)+(DSIZE-1)) / (DSIZE));
    }
    if((bp = find_fit(asize))!= NULL){
        place(bp,asize);
        //mm_check("alloc\n");
        return bp;
    }
    extendsize = MAX(asize,CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE))==NULL){
        return NULL;
    }
    place(bp,asize);
    //mm_check("alloc after extending mem\n");
    return bp;
}

/*
 * mm_free
 */
void mm_free(void *bp)
{

    if(bp == 0)
	return;
    size_t size = GET_SIZE(HDRP(bp));
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    SET_PRED(bp, 0);
    SET_SUCC(bp, 0);
    //insert(bp);
    coalesce(bp);
    //mm_check("free\n");
}
static void delete_block(void *bp){
  
    void *next = GET_SUCC(bp);
    void *prev = GET_PRED(bp);


    if(!prev){  // the block to be deleted is the head
        if(next){
            SET_PRED(next, 0);
            SET_SUCC(next, 0);
            head = next;  // change head to the next
        }
        else 
		{
			head = NULL;

		}
    }
    else{
        if(next) SET_PRED(next, prev);
        SET_SUCC(prev, next);
    }
    SET_SUCC(bp, 0);
    SET_PRED(bp, 0);
    return;
} 
static void *coalesce(void *bp){
    size_t  prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t  next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) {     
        insert(bp);
        return bp;
    }
    else if(prev_alloc && !next_alloc){
        //printf("CASE 2 in coalesce\n");//just test
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_block(NEXT_BLKP(bp));  // delete next node
        delete_block(bp);
        PUT(HDRP(bp), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
        
    }
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        delete_block(PREV_BLKP(bp));   // delete previous node
       	delete_block(bp);
		void *prev = PREV_BLKP(bp);
        PUT(FTRP(bp),PACK(size,0));
        PUT(HDRP(prev),PACK(size,0));
		bp = prev;
    }
    else {
      
        //printf("CASE 4 in coalesce\n");//just test
        char *next = NEXT_BLKP(bp);
        char *prev = PREV_BLKP(bp);
        size += GET_SIZE(FTRP(next)) + GET_SIZE(HDRP(prev));
        delete_block(bp);
        delete_block(next);
        delete_block(prev);
        PUT(FTRP(next),PACK(size,0));
		PUT(HDRP(prev),PACK(size,0));
        bp = prev;
    }
    
    insert(bp);
    return bp;
}
static void *insert(void *bp){
    if(head != 0){
        long *p = head; 
        SET_PRED(p, bp);
        SET_SUCC(bp, p);
        printf("p%u bp%u\n",p,bp);
        head = bp;
    }
    else head = bp;
}

static void *find_fit(size_t size){ 
      // first-fit
    if(head == NULL) return NULL;
    void *searchp = head;
	void *previous ;
    while(searchp != NULL){

		if(GET_SIZE(HDRP(searchp))>=size){
            return searchp;
        }
		previous = searchp;
        searchp = GET_SUCC(searchp);
		if(searchp == previous)
		{
			return NULL;	
    	}
	}
    return NULL;
    
}
static void place(void *bp,size_t asize){

    size_t csize = GET_SIZE(HDRP(bp));
    delete_block(bp);
    if((csize-asize)>=(3*DSIZE)){
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        SET_SUCC(bp, 0);
        SET_PRED(bp, 0);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp),PACK(csize-asize,0));
        PUT(FTRP(bp),PACK(csize-asize,0));
        if(head != NULL){
            void *p = head; 
            //if(p != NULL){
            SET_PRED(p, bp);
            //}
            SET_SUCC(bp, p);
			SET_PRED(bp, 0);
            head = NULL;
        }
        else{
            //head = bp;
            SET_PRED(bp, 0);
            SET_SUCC(bp, 0);
        }
        coalesce(bp);
        
        
    }
    else // small block, do not split
    {
        PUT(HDRP(bp),PACK(csize,1));
        PUT(FTRP(bp),PACK(csize,1));
        SET_SUCC(bp, 0);
        SET_PRED(bp, 0);
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

int mm_check(char func[]){
    // check the free list
    printf("=====cur function:%s ",func);
    //print all the blocks
    for(void *bp = heap_listp; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
        printf("bp:%u\n",bp);
        printf("block size:%u\n",GET_SIZE(HDRP(bp)));
        printf("block allocated or not:%u\n",GET_ALLOC(HDRP(bp)));
        printf("if not allocated pred free block: %u\n", GET_PRED(bp));
        printf("if not allocated succ free block: %u\n", GET_SUCC(bp));
        printf("-----head: %u-----\n", head);
    }

    return 0;
};
