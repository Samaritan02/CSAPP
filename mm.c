/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = { "", "", "", "", "" };

//Some global variables
char *heap = 0;// point to the very first byte of heap
char **seglist;// segregated list storing the free blocks

//Some essential functions I shall use
void *expand(size_t size); // expand the heap if necessary
void *coalesce(void *BlockPointer); // join the free blocks if necessary
void insert(void *BlockPointer, size_t size);
void delete(void *BlockPointer);
void *place(void *BlockPointer, size_t size);

//Some basic macros according to CSAPP
#define WORD 4 // size for a word
#define DWORD 8 // size for a double word
#define InitHeap 64 // default heap size
#define AddHeap 4096 // additional heap size once needed
#define MAXNUM 16


#define TAG(size, alloc) ((size) | (alloc)) // use one word to store the size and allocation bit

#define GET(p) (*(unsigned int *)(p)) // read an address
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // write an address

#define GetSize(p) (GET(p) & (-8)) // notice that there is trash bits due to alignment
#define GetAlloc(p) ((GET(p)) & (1)) // get the allocation bit 

#define HEADER(BlockPointer) ((char *)(BlockPointer) - WORD) // notice that header must be followed by the first word of block
#define FOOTER(BlockPointer) ((char *)(BlockPointer) + GetSize(HEADER(BlockPointer)) - DWORD) //according to CSAPP about header, payload and footer

#define NEXT_BLKP(bp) ((char *)(bp)+GetSize(((char *)(bp)-WORD)))// the same with CSAPP
#define PREV_BLKP(bp) ((char *)(bp)-GetSize(((char *)(bp)-DWORD)))// the same with CSAPP

// notice that (char *) can get a whole line, i.e. a free memory set; while *(char **) can get a certain value, i.e. a specific free memory

#define PredPtr(BlockPointer) ((char *)(BlockPointer))
#define SuccPtr(BlockPointer) ((char *)(BlockPointer) + WORD)

#define PRED(BlockPointer) (*(char **)(BlockPointer))
#define SUCC(BlockPointer) (*(char **)(SuccPtr(BlockPointer)))

// other useful macros
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define SetPointer(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))// set pointer to p

#define GetFree(i) (*(seglist + i))
#define SetFree(i, BlockPointer) (*(seglist + i) = BlockPointer)


/*
* insert a new free block into the seglist(details are explained below)
*/
void insert(void *BlockPointer, size_t size)
{
    int which = 0; // stroing which size class should it be stored
    size_t target = size; // just for determining the position(might be a problem)
    // once this loop breaks, we know where the block should store
    while ((which < MAXNUM - 1) && (target > 1))
    {
        which++;
        target >>= 1;
    }

    void *cur = GetFree(which);// get the size class pointer
    void *succ = NULL;// always point to the successor of current node
    //maintain the order so that "hit" would be quick and approriate
    while((cur != NULL) && (size > GetSize(HEADER(cur))))
    {
        succ = cur;
        cur = PRED(cur);
    }

    // this is the biggest block existed
    if(cur == NULL)
    {
        if (succ == NULL)// there is no block at all
        {
            SetPointer(PredPtr(BlockPointer), NULL);
            SetPointer(SuccPtr(BlockPointer), NULL);
            SetFree(which, BlockPointer);
        }
        else // put it to the very beginning 
        {
            SetPointer(PredPtr(BlockPointer), NULL);
            SetPointer(SuccPtr(BlockPointer), succ);
            SetPointer(PredPtr(succ), BlockPointer);
        }
    }
    else
    {
        if(succ == NULL)// the smallest block
        {
            SetPointer(PredPtr(BlockPointer), cur);
            SetPointer(SuccPtr(cur), BlockPointer);
            SetPointer(SuccPtr(BlockPointer), NULL);
            SetFree(which, BlockPointer);
        }
        else // the most ordinary situation, put it between succ and cur
        {
            SetPointer(PredPtr(BlockPointer), cur);
            SetPointer(SuccPtr(cur), BlockPointer);
            SetPointer(SuccPtr(BlockPointer), succ);
            SetPointer(PredPtr(succ), BlockPointer);
        }
    }
}

/*
* delete an existing block so that we can coalesce
*/
void delete(void *BlockPointer)
{
    int which = 0;// exactly the same with insert
    size_t target = GetSize(HEADER(BlockPointer));// determining the position
    while((which < MAXNUM -1) && (target > 1))
    {
        which++;
        target >>= 1;
    }

    if(PRED(BlockPointer) == NULL)
    {
        if(SUCC(BlockPointer) == NULL)// there is only the target block 
        {
            SetFree(which, PRED(BlockPointer));
        }
        else // no predecessor, with successor
        {
            SetPointer(PredPtr(SUCC(BlockPointer)), NULL);// delete directly
            //might be a problem
        }
    }
    else
    {
        if(SUCC(BlockPointer) == NULL)// no successor, with predecessor
        {
            SetPointer(SuccPtr(PRED(BlockPointer)), NULL);
            SetFree(which, PRED(BlockPointer));
        }
        else // with predecessor and successor
        {
            // link the predecessor and successor
            SetPointer(SuccPtr(PRED(BlockPointer)), SUCC(BlockPointer));
            SetPointer(PredPtr(SUCC(BlockPointer)), PRED(BlockPointer));
        }
    }
}

//This is the same with CSAPP with my understanding of its usage line by line
void *coalesce(void *bp)
{
    size_t prev_alloc = GetAlloc(HEADER(PREV_BLKP(bp)));
    size_t next_alloc = GetAlloc(HEADER(NEXT_BLKP(bp)));
    size_t size = GetSize(HEADER(bp));

    if(prev_alloc && next_alloc) // Case 1
    {
        return bp;
    }
    else if(prev_alloc && !next_alloc) // Case 2
    {
        // remove the coalescing blocks
        delete (bp);
        delete (NEXT_BLKP(bp));
        // join the blocks into 1
        size += GetSize(HEADER(NEXT_BLKP(bp)));
        PUT(HEADER(bp), TAG(size, 0));
        PUT(FOOTER(bp), TAG(size, 0));
    }
    else if(!prev_alloc && next_alloc) // Case 3
    {
        // remove the coalescing blocks
        delete (bp);
        delete (PREV_BLKP(bp));
        // join the blocks into 1
        size += GetSize(HEADER(PREV_BLKP(bp)));
        // Notice that the FOOTER remain the same while the HEADER has changed
        PUT(FOOTER(bp), TAG(size, 0));
        PUT(HEADER(PREV_BLKP(bp)), TAG(size, 0));
        bp = PREV_BLKP(bp);
    }
    else // Case 4
    {
        // remove the coalescing blocks
        delete (bp);
        delete (NEXT_BLKP(bp));
        delete (PREV_BLKP(bp));
        // join the blocks into 1
        size += GetSize(HEADER(NEXT_BLKP(bp))) + GetSize(HEADER(PREV_BLKP(bp)));
        PUT(HEADER(PREV_BLKP(bp)), TAG(size, 0));
        PUT(FOOTER(NEXT_BLKP(bp)), TAG(size, 0));
        bp = PREV_BLKP(bp);
    }

    // put the new free block into seglist
    insert(bp, size);
    return bp;
}

void *place(void *BlockPointer, size_t size)
{
    size_t RestSize = GetSize(HEADER(BlockPointer)) - size;
    delete (BlockPointer); // this free block are destinied to be removed, so remove first to save possible search time

    if(RestSize <= 2 * DWORD) // too small, no need to split
    {
        size_t TmpSize = GetSize(HEADER(BlockPointer));
        PUT(HEADER(BlockPointer), TAG(TmpSize, 1));
        PUT(FOOTER(BlockPointer), TAG(TmpSize, 1));
    }
    else if (size >= 96)
    {
        PUT(HEADER(BlockPointer), TAG(RestSize, 0));
        PUT(FOOTER(BlockPointer), TAG(RestSize, 0));
        PUT(HEADER(NEXT_BLKP(BlockPointer)), TAG(size, 1));
        PUT(FOOTER(NEXT_BLKP(BlockPointer)), TAG(size, 1));
        insert(BlockPointer, RestSize);
        return NEXT_BLKP(BlockPointer);
    }
    else// might be a problem
    {
        PUT(HEADER(BlockPointer), TAG(size, 1));
        PUT(FOOTER(BlockPointer), TAG(size, 1));
        PUT(HEADER(NEXT_BLKP(BlockPointer)), TAG(RestSize, 0));
        PUT(FOOTER(NEXT_BLKP(BlockPointer)), TAG(RestSize, 0));
        insert(NEXT_BLKP(BlockPointer), RestSize);// remember to put the remaining free block into the seglist
    }
    return BlockPointer;
}

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 * set the seglist and the heap(details are explained below)
 */
int mm_init(void)
{
    // seperate the free blocks into 16 possible sets
    if((seglist = mem_sbrk(MAXNUM * sizeof(char *))) == (void *)(-1))
    {
        return -1;
    }
    // initialize the seglist
    for (int i = 0; i < MAXNUM; i++)
    {
        SetFree(i, NULL);
    }
    // initialize the heap with 4 words
    if((heap = mem_sbrk(4 * WORD)) == (void*) (-1))
    {
        return -1;
    }

    PUT(heap, 0);
    // always tag preface and ending block as used(like the head node and tail node in the linked list), so that
    // I don't need to care the edge case
    PUT(heap + WORD, TAG(DWORD, 1));// header of the preface
    PUT(heap + 2 * WORD, TAG(DWORD, 1));// footer of the preface
    PUT(heap + 3 * WORD, TAG(0, 1));// header of the ending block
    heap = heap + 2 * WORD;             // reset the start of the heap;

    // expand the heap so that there is real free blocks
    if(expand(InitHeap) == NULL)
    {
        return -1;
    }

    return 0;
}

// the following function "expand" is basically the same with CSAPP
// with my understanding of its usage line by line
void *expand(size_t size)
{
    void *bp;//short for BlockPointer
    size_t Aligned_Size = ALIGN(size);// align the size 

    // expand unsuccessfully
    if((bp = mem_sbrk(Aligned_Size)) == (void*)(-1))
    {
        return NULL;
    }
    // otherwise, we set the header, footer and ending block

    PUT(HEADER(bp), TAG(Aligned_Size, 0));// unused block
    PUT(FOOTER(bp), TAG(Aligned_Size, 0));// unused block
    PUT(HEADER(NEXT_BLKP(bp)), TAG(0, 1));// update the ending block
    insert(bp, Aligned_Size);// insert the new free block into the seglist

    return coalesce(bp); // coalesce if necessary
}

/* 
 * This is the naive method
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

/*
* This is my method(seglist)
* First align the size 
* Then find the size class available, or apply for more memory
*/
void *mm_malloc(size_t size)
{
    size_t AlignedSize;
    void *bp = NULL;
    if(size == 0)// no application at all
    {
        return NULL;
    }

    // otherwise, first align the size
    if (size <= DWORD) // need 8 bytes to store HEADER and FOOTER, so the block can't be too small
    {
        AlignedSize = 2 * DWORD;
    }
    else
    {
        AlignedSize = ALIGN(size + DWORD);
    }

    int which = 0;// just the same with insert and delete, find the proper size class
    size_t target = AlignedSize;// also the same

    while(which < MAXNUM)
    {
        // either we already hit the last of seglist or teh target fall in a nonempty size class
        if((which == MAXNUM - 1) || ((target <= 1) && (GetFree(which) != NULL)))
        {
            bp = GetFree(which);
            while((bp != NULL) && (AlignedSize > GetSize(HEADER(bp))))// find the proper free block
            {
                bp = PRED(bp);
            }
            if(bp != NULL)
            {
                return place(bp, AlignedSize);
            }
        }

        target >>= 1;
        which++;
    }
    // there is no proper free block, we need additional space
    if(bp == NULL)
    {
        size_t AddSize = MAX(AlignedSize, AddHeap);
        if((bp = expand(AddSize)) == NULL)// bad expansion
        {
            return NULL;
        }
    }

    bp = place(bp, AlignedSize);
    return bp;

    /*int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }*/
}

/*
 * mm_free - Freeing a block does nothing.
 */

/*
* First check the validation of ptr
* then check if heap is not initiated
* finally do the free job, put into the seglist and coalesce if necessary
*/

void mm_free(void *ptr)
{
    if(ptr == 0)
    {
        return;
    }
    size_t FreeSize = GetSize(HEADER(ptr));
    if(heap == 0)
    {
        mm_init();
    }

    PUT(HEADER(ptr), TAG(FreeSize, 0));// tag the free block
    PUT(FOOTER(ptr), TAG(FreeSize, 0));

    insert(ptr, FreeSize);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

/*
* According to the handout and my understanding, there are actually 4 situations:
* 1. if the block is NULL, equal to alloc
* 2. if the size is 0, equal to free
* 3. if the primitive size is larger than size, do nothing
* 4. apply for a new block and copy the text(notice that if the next block is empty, we can expand the current block)
*/
void *mm_realloc(void *ptr, size_t size)
{
    void *newptr = ptr;
    size_t CopySize;

    if(ptr == NULL) // Case 1
    {
        return mm_malloc(size);
    }

    if(size == 0) // Case 2
    {
        mm_free(ptr);
        return NULL;
    }

    if(size <= DWORD) // if the block is too small
    {
        size = 2 * DWORD;
    }
    else
    {
        size = ALIGN(size + DWORD);
    }

    if(GetSize(HEADER(ptr)) >= size) // Case 3
    {
        return ptr;
    }
    else if ((!GetAlloc(HEADER(NEXT_BLKP(ptr)))) && (CopySize = GetSize(HEADER(ptr))+ GetSize(HEADER(NEXT_BLKP(ptr))) -size )) // Case 4(expand)
    {
        delete (NEXT_BLKP(ptr));

        PUT(HEADER(ptr), TAG(size + CopySize, 1));
        PUT(FOOTER(ptr), TAG(size + CopySize, 1));
    }
    else // Case 4(apply for a new block)
    {
        newptr = mm_malloc(size);
        memcpy(newptr, ptr, GetSize(HEADER(ptr)));
        mm_free(ptr);
    }
    return newptr;

    /*void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;*/

}


// The following code is heap checker.
// Here are some functions I might use later

int mm_check();
int CheckBlock(void *bp);
void PrintBlock(void *bp);
int CheckList(void *i);
void PrintList(void *i);

/*
* The heap checker has the following functions:
* 1. print the heap blocks and check if they're aligned, valid, etc.(every possible mistakes I can think of, details explained afterwards)
* 2. print the seglist and check if they're linked correctly(or other mistakes I can think of, details explained afterwards)
* 3. check if there are other mistakes that might occur 
*/
int mm_check(void)
{
    // check if there are continuous free blocks
    int pred_free = 0, cur_free = 0;
    for (char *bp = heap; GetSize(HEADER(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        PrintBlock(bp);
        pred_free = cur_free;
        cur_free = CheckBlock(bp);
        if(pred_free && cur_free)
        {
            printf("There are continuous free blocks.\n");
            return 0;
        }
    }

    // print and check the seglist
    for (int i = 0; i < MAXNUM; i++)
    {
        PrintList(GetFree(i));
        if(CheckList(GetFree(i)))
        {
            return 0;
        }
    }

    return 1;
}

int CheckBlock(void* bp)
{
    if((size_t)bp % 8)// check if aligned
    {
        printf("%p Not Aligned.\n", bp);
    }

    if(GET(HEADER(bp)) != GET(FOOTER(bp))) // check if the HEADER and FOOTER match
    {
        printf("%p header and footer don't match.\n", bp);
    }

    if(GetSize(HEADER(bp)) % 8)// check the payload size
    {
        printf("%p payload size is not aligned", bp);
    }

    return GetAlloc(HEADER(bp));
}

void PrintBlock(void *bp)
{
    int halloc, falloc;// HEADER and FOOTER alloc tag
    int hsize, fsize;// HEADER and FOOTER size tag

    halloc = GetAlloc(HEADER(bp));
    falloc = GetAlloc(FOOTER(bp));
    hsize = GetSize(HEADER(bp));
    fsize = GetSize(FOOTER(bp));

    printf("%p: header: [%d:%d] footer: [%d:%d]", bp, hsize, halloc, fsize, falloc);
    //Notice in small trace, check every block manually is much faster than writing a loop
    //to check the block information since there is always unexpected mistakes.
}

int CheckList(void *i)
{
    void *pred = NULL;
    int halloc, hsize;
    for (; i != NULL; i = SUCC(i))
    {
        //the following two ifs are to check whether the seglist is linked correctly
        if(PRED(i) != pred)
        {
            printf("predecessor point is wrong.\n");
            return 1;
        }
        if(pred != NULL && SUCC(pred) != i)
        {
            printf("successor point is wrong.\n");
            return 1;
        }

        hsize = GetSize(HEADER(i));
        halloc = GetAlloc(HEADER(i));
        if(halloc)// wrongly put allocated block into seglist
        {
            printf("This block should not be put into seglist.\n");
            return 1;
        }
        if(pred != NULL && (GetSize(HEADER(pred)) > hsize))// check if the seglist is in the right order
        {
            printf("Not Sorted!\n");
            return 1;
        }
        pred = i;
    }
    return 0;
}

void PrintList(void *i)
{
    int hsize, halloc;
    for (; i != NULL; i = SUCC(i))
    {
        hsize = GetSize(HEADER(i));
        halloc = GetAlloc(HEADER(i));
        printf("%p: header: [%d:%d] pred: [%p] succ: [%p]\n ", i, hsize, halloc, PRED(i), SUCC(i));
        // Again, simply printing the block would help me find the unexpected mistakes.
    }
}











