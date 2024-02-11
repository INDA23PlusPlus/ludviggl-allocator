
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
struct block *__b_start;
// points to the end of last block in memory
struct block *__b_end;
// points to the next block to consider for allocation
struct block *__b_next;



__attribute__((constructor))
static void __b_init(void)
{
    // setup an inital block of size BUDDY_BLOCK_INIT_SIZE

    __b_start = sbrk(0);
    if (sbrk(BUDDY_BLOCK_INIT_SIZE) == (void *) -1)
    {
        assert(0 && "failed to initialize buddy.h");
    }
    __b_end = (struct block *)
        ((byte_t *) __b_start + BUDDY_BLOCK_INIT_SIZE);

    __b_next = __b_start;
    __b_next->size = BUDDY_BLOCK_INIT_SIZE;
    __b_next->used = 0;
}

static struct block *__b_grow(size_t required)
{
    required = BLOCKSIZE(required);
    size_t current_size = BYTEDIFF(__b_start, __b_end);
    struct block *block;

    // keep growing until last block is big enough
    do {

        if (sbrk(current_size) == (void *) -1)
        {
            return BNULL;
        }

        block = __b_end;
        block->size = current_size;
        block->used = 0;

        __b_end = (struct block *)
            ((byte_t *) block + current_size);

        current_size = BYTEDIFF(__b_start, __b_end);
    } while (current_size < required);

    return block;
}

static void __b_split(struct block *block)
{
    block->size /= 2;
    struct block *next = NEXT(block);
    next->size = block->size;
    next->used = 0;
}

static struct block *__b_join(struct block *block)
{
    struct block *buddy, *new_block;

    // find buddy
    size_t offset = (size_t)
        ((byte_t *) block - (byte_t *)__b_start);
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
    if (buddy == __b_end ||
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
    struct block *block = __b_next;

    // search for unused block that fits allocation
    while (MEMSIZE(block) < size || block->used)
    {

        block = NEXT(block);

        // wrap around
        if (block == __b_end)
        {
            block = __b_start;
        }

        // went through all available blocks
        // try to grow
        if (block == __b_next)
        {
            block = __b_grow(size);
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
        __b_split(block);
    }

    // record where we should start searching next
    __b_next = NEXT(block);
    if (__b_next == __b_end)
    {
        __b_next = __b_start;
    }

    block->used = 1;
    return block->mem;
}

void bfree(void *ptr)
{
    struct block *block = BLOCK(ptr);
    block->used = 0;

    // join unused buddies
    while ((block = __b_join(block)) != BNULL)
    {
        //  // make sure __n_next doesn't point to
        //  // a block removed during joining
        //  if (BYTEDIFF(block, __b_next) < block->size)
        //  {
        //      __b_next = block;
        //  }

        // this block is definitely unused
        __b_next = block;
    }
}

#endif
