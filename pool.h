
#ifndef POOL_H
#define POOL_H

/*
 * Pool allocator
 *
 * Usage:
 *
 *     #define POOL_BLOCK_SIZE 128
 *     #define POOL_IMPLEMENTATION
 *     #include "pool.h"
 *
 *     int main()
 *     {
 *         void * ptr = palloc();
 *         pfree (ptr);
 *     }
 *
 */

#define PNULL ((void *) 0)

/*
 * Allocates a new memory block of size POOL_BLOCK_SIZE.
 * Returns PNULL on failure.
 */
void * palloc (void);

/*
 * Frees memory pointed to by `ptr`
 */
void pfree (void * ptr);

#endif

#ifdef POOL_IMPLEMENTATION
#undef POOL_IMPLEMENTATION

#include <stddef.h>
#include <unistd.h>
#include <stdint.h>

#ifndef POOL_BLOCK_SIZE
#error POOL_BLOCK_SIZE must be defined.
#endif

#define PROGRAM_BREAK_INCREMENT 4096
#if POOL_BLOCK_SIZE > PROGRAM_BREAK_INCREMENT
#undef PROGRAM_BREAK_INCREMENT
#define PROGRAM_BREAK_INCREMENT (POOL_BLOCK_SIZE)
#endif

#define BLOCK_SIZE (POOL_BLOCK_SIZE)

union block
{
    union block * next_free;

    _Alignas(max_align_t)
    uint8_t mem[POOL_BLOCK_SIZE];
};

_Static_assert((BLOCK_SIZE & (BLOCK_SIZE - 1)) == 0,
               "POOL_BLOCK_SIZE must be a power of two");

static union block * __free_head;
static union block * __front;
static union block * __end;

__attribute__((constructor))
static void __init (void)
{
    __free_head = PNULL;
    __front = (union block *) sbrk(0);
    __end = __front;
}

static int __more (void)
{
    if (sbrk(PROGRAM_BREAK_INCREMENT) == (void *) -1)
    {
        return 0;
    }

    __end = (union block *) sbrk(0);
    return 1;
}

static void * __pop_free (void)
{
    void * ptr = (void *) __free_head;
    __free_head = __free_head->next_free;
    return ptr;
}

void * palloc (void)
{
    if (__free_head)
    {
        return __pop_free();
    }

    if (__front + BLOCK_SIZE > __end && !__more())
    {
        return PNULL;
    }

    return (void *) __front++;
}

void pfree (void * ptr)
{
    ((union block *) ptr)->next_free = __free_head;
    __free_head = (union block *) ptr;
}

#endif
