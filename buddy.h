
/**
 *  Buddy allocator. Do not use in conjunction with
 *  malloc / free.
 *
 *  Usage:
 *
 *      #define BUDDY_IMPLEMENTATION
 *      #include "buddy.h"
 *
 *      int main()
 *      {
 *          char *ptr = balloc(sizeof(*ptr));
 *          *ptr = 'X';
 *          bfree(ptr);
 *      }
 */

#ifndef BUDDY_H
#define BUDDY_H

#include <stddef.h>

#define BNULL ((void *) 0)

/**
 *  Allocate `size` bytes of memory.
 *  Returns BNULL on failure.
 */
void *balloc(size_t size);

/**
 *  Free previously allocated memory.
 */
void bfree(void *ptr);

#endif



#ifdef BUDDY_IMPLEMENTATION
#undef BUDDY_IMPLEMENTATION

#include <stdint.h>
#include <unistd.h>
#include <assert.h>

typedef uint8_t byte_t;

struct block {
    size_t size;
    int used;
    _Alignas(max_align_t) byte_t mem[];
};

#ifndef BUDDY_BLOCK_INIT_SIZE
#define BUDDY_BLOCK_INIT_SIZE 4096
#endif



// find next block
#define NEXT(block_ptr)\
    (struct block *)((byte_t*)(block_ptr) + (block_ptr)->size)
// the offset of the usable memory region in a block
#define MEMOFFSET (offsetof(struct block, mem))
// the size of usable memory `block_ptr` can hold
#define MEMSIZE(block_ptr) (size_t)((block_ptr)->size - MEMOFFSET)
// get pointer to block containing `mem`
#define BLOCK(mem) (struct block *)((byte_t *)mem - MEMOFFSET)
// the size of usable memory if we would split `block_ptr`
#define HALFMEMSIZE(block_ptr)\
    (size_t)((block_ptr)->size / 2 - MEMOFFSET)
// the smallest size a block can be
#define MINBLOCKSIZE 16
// the size of a block that can hold `memsize` bytes
// of usable memory
#define BLOCKSIZE(memsize) (memsize + MEMOFFSET)
// the byte offset between two pointers
#define BYTEDIFF(ptr1, ptr2)\
    (size_t) ((byte_t *) ptr2 - (byte_t *) ptr1)



// points to the first block in memory
static struct block *start;
// points to the end of last block in memory
static struct block *end;
// points to the next block to consider for allocation
static struct block *next;



__attribute__((constructor))
static void init(void)
{
    // setup an inital block of size BUDDY_BLOCK_INIT_SIZE

    start = sbrk(0);
    if (sbrk(BUDDY_BLOCK_INIT_SIZE) == (void *) -1)
    {
        assert(0 && "failed to initialize buddy.h");
    }
    end = (struct block *)
        ((byte_t *) start + BUDDY_BLOCK_INIT_SIZE);

    next = start;
    next->size = BUDDY_BLOCK_INIT_SIZE;
    next->used = 0;
}

static struct block *grow(size_t required)
{
    required = BLOCKSIZE(required);
    size_t current_size;
    struct block *block;

    // keep growing until last block is big enough
    do {
        current_size = BYTEDIFF(start, end);

        if (sbrk(current_size) == (void *) -1)
        {
            return BNULL;
        }

        block = end;
        block->size = current_size;
        block->used = 0;

        end = NEXT(block);

    } while (current_size < required);

    return block;
}

static void split(struct block *block)
{
    block->size /= 2;
    struct block *next = NEXT(block);
    next->size = block->size;
    next->used = 0;
}

static struct block *join(struct block *block)
{
    struct block *buddy, *new_block;

    // find buddy
    size_t offset = (size_t)
        ((byte_t *) block - (byte_t *)start);
    if (offset % (block->size * 2) == 0)
    {
        buddy = NEXT(block);
        new_block = block;
    }
    else
    {
        buddy = (struct block *) ((byte_t *) block - block->size);
        new_block = buddy;
    }

    // can we join?
    if (buddy == end ||
        buddy->size != block->size ||
        buddy->used)
    {
        return BNULL;
    }

    new_block->size *= 2;
    new_block->used = 0;

    return new_block;
}

void *balloc(size_t size)
{
    struct block *block = next;

    // search for unused block that fits allocation
    while (MEMSIZE(block) < size || block->used)
    {

        block = NEXT(block);

        // wrap around
        if (block == end)
        {
            block = start;
        }

        // went through all available blocks
        // try to grow
        if (block == next)
        {
            block = grow(size);
            if (block == BNULL)
            {
                // can't grow
                return BNULL;
            }
            break;
        }
    }

    // split until we have best fit
    while (HALFMEMSIZE(block) >= size && block->size > MINBLOCKSIZE)
    {
        split(block);
    }

    // record where we should start searching next
    next = NEXT(block);
    if (next == end)
    {
        next = start;
    }

    block->used = 1;
    return block->mem;
}

void bfree(void *ptr)
{
    struct block *block = BLOCK(ptr);
    block->used = 0;

    // join unused buddies
    while ((block = join(block)) != BNULL)
    {
        //  // make sure __n_next doesn't point to
        //  // a block removed during joining
        //  if (BYTEDIFF(block, next) < block->size)
        //  {
        //      next = block;
        //  }

        // this block is definitely unused
        next = block;
    }
}

#endif
