
/* Arena allocator
 *
 * Example usage:
 *
 *     #include <assert.h>
 *
 *     #define ARENA_IMPLEMENTATION
 *     #include "arena.h"
 *
 *     int main()
 *     {
 *         arena_t A;
 *         assert(arena_init(&A, 4096) != -1);
 *
 *         void *ptr = arena_alloc(&A, 128);
 *         arena_free(&A);
 *     }
 *
 */

#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t front;
    size_t size;
    uint8_t *mem;
} arena_t;

/*
 * Initializes a new arena of size `size`
 * using malloc, or ARENA_MALLOC if ARENA_NO_STDLIB
 * is defined. Returns -1 on failure and 0 otherwise.
 */
int arena_init(arena_t *A, size_t size);

/*
 * Initializes a new arena using preallocated memory.
 */
void arena_init_prealloc(arena_t *A, void *mem, size_t size);

/*
 * Frees underlying memory using free, or
 * ARENA_FREE if ARENA_NO_STDLIB is defined.
 */
void arena_free(arena_t *A);

/*
 * Frees arena without freeing underlying memory.
 */
void arena_clear(arena_t *A);

/*
 * Allocates a region of size `size`.
 * Returns NULL on failure, or ARENA_NULL
 * if ARENA_NO_STDLIB is defined.
 */
void *arena_alloc(arena_t *A, size_t size);

#endif

#ifdef ARENA_IMPLEMENTATION
#define ARENA_IMPLEMENTATION

#ifdef ARENA_NO_STDLIB
    #ifndef ARENA_MALLOC
        #error ARENA_MALLOC must be defined
    #endif
    #ifndef ARENA_FREE
        #error ARENA_FREE must be defined
    #endif
    #ifndef ARENA_NULL
        #error AERNA_NULL must be define
    #endif
#else
    #include <stdlib.h>
    #define ARENA_MALLOC(size) malloc(size)
    #define ARENA_FREE(ptr) free(ptr)
    #define ARENA_NULL NULL
#endif

int arena_init(arena_t *A, size_t size)
{
    A->front = 0;
    A->size = size;
    A->mem = ARENA_MALLOC(size);
    return -(A->mem == ARENA_NULL);
}

void arena_free(arena_t *A)
{
    A->size = 0;
    A->front = 0;
    ARENA_FREE(A->mem);
    A->mem = ARENA_NULL;
}

void arena_init_prealloc(arena_t *A, void *mem, size_t size)
{
    A->front = 0;
    A->size = size;
    A->mem = mem;
}

void arena_clear(arena_t *A)
{
    A->front = 0;
}

void *arena_alloc(arena_t *A, size_t size)
{
    if (A->front + size > A->size)
    {
        return ARENA_NULL;
    }

    void *ptr = &A->mem[A->front];
    A->front += size;
    return ptr;
}

#endif
