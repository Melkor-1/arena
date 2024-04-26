/* A consideration for future:
 *
 * The `is_multiple_of()` function essentially limits the types of allocations
 * that a user can make to even size (if desired alignment is more than 1).
 * There may be valid use cases for allocating memory that is not a multiple
 * of the alignment; a user may want to allocate an object aligned to the cache
 * boundary (which is 64 or 128 bytes on modern systems). */

#include "arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>

/* *INDENT-OFF* */
/* In C2X/C23 or later, nullptr is a keyword. */
/* Patch up C18 (__STDC_VERSION__ == 201710L) and earlier versions.  */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L
    #define nullptr ((void *)0)
#endif

#define INITIAL_MPOOL_COUNT  2

#ifdef DEBUG
    #include <string.h>
    #define D(x) x
#else
    #define D(x) (void) 0
#endif
/* *INDENT-ON* */

typedef struct pool {
    size_t offset;
    size_t buf_len;
    bool is_heap_alloc;
    uint8_t *buf;
} M_Pool;

struct arena {
    size_t count;
    size_t capacity;
    size_t current;
    size_t last_alloc_size;
    M_Pool *pools[];
};

ATTRIB_INLINE ATTRIB_CONST static inline bool is_power_of_two(uintptr_t x)
{
    return (x & (x - 1)) == 0;
}

ATTRIB_INLINE ATTRIB_CONST static inline bool is_multiple_of(size_t a, size_t b)
{
    return a % b == 0;
}

size_t arena_pool_capacity(Arena *arena)
{
    const M_Pool *const curr_pool = arena->pools[arena->current - 1];

    return curr_pool->buf_len - curr_pool->offset;
}

size_t arena_allocated_bytes(Arena *arena)
{
    size_t sum = 0;

    for (size_t i = 0; i < arena->count; ++i) {
        sum += arena->pools[i]->buf_len;
    }

    return sum;
}

size_t arena_allocated_bytes_including_metadata(Arena *arena)
{
    return offsetof(Arena, pools)
        + sizeof arena->pools[0] * arena->capacity
        + arena_allocated_bytes(arena);
}

static M_Pool *pool_new(void *buf, size_t capacity)
{
    M_Pool *const pool = calloc(1, sizeof *pool);

/* *INDENT-OFF* */
    if (pool != nullptr) {
        *pool = (M_Pool) {
            .buf_len = capacity,
            .is_heap_alloc = buf == nullptr,
            .buf = buf ? buf : calloc(1, capacity),
        };
    }
/* *INDENT-ON* */

    return pool;
}

Arena *arena_new(void *buf, size_t capacity)
{
    if (capacity == 0) {
        if (buf != nullptr) {
            return nullptr;
        }
        capacity = DEFAULT_BUF_CAP;
    }

    Arena *const arena = calloc(1,
        sizeof *arena + (INITIAL_MPOOL_COUNT * sizeof arena->pools[0]));

    if (arena == nullptr) {
        return nullptr;
    }

    arena->capacity = INITIAL_MPOOL_COUNT;
    arena->count = 1;
    arena->current = 1;
    arena->pools[0] = pool_new(buf, capacity);

    if (arena->pools[0] == nullptr) {
        free(arena);
        return nullptr;
    }

    return arena;
}

void *arena_alloc(Arena *arena, size_t alignment, size_t size)
{
    if (size == 0
        || alignment == 0 || (alignment != 1 && !is_power_of_two(alignment))
        || !is_multiple_of(size, alignment)) {
        return nullptr;
    }

    M_Pool *curr_pool = arena->pools[arena->current - 1];
    uint8_t *const p = curr_pool->buf + curr_pool->offset;
    const uintptr_t original = ((uintptr_t) p);

    if (original > UINTPTR_MAX - alignment) {
        return nullptr;
    }

    const uintptr_t remain = original & (alignment - 1);
    const uintptr_t aligned =
        remain != 0 ? original + (alignment - remain) : original;
    const size_t offset = aligned - original;

    if (size > SIZE_MAX - offset) {
        return nullptr;
    }

    size += offset;

    if (size > curr_pool->buf_len - curr_pool->offset) {
        return nullptr;
    }

    /* Set the optional padding for alignment immediately before a user block, 
     * and the bytes immediately following such a block to non-zero. 
     * The intent is to trigger OBOB failures to inappropiate app use of 
     * strlen()/strnlen(), which keep forging ahead till encountering ascii NUL. */
    D(
        /* 0xA5 is used in FreeBSD's PHK malloc for debugging purposes. */
        if (remain) {
            memset(p + (alignment - remain), 0xA5, alignment - remain);
        }
    );

    curr_pool->offset += size;
    D(memset(curr_pool->buf + curr_pool->offset, 0xA5,
            curr_pool->buf_len - curr_pool->offset));

    arena->last_alloc_size = size;

    /* Equal to "aligned", but preserves provenance. */
    return p + offset;
}

void *arena_allocarray(Arena *arena,
                       size_t alignment, 
                       size_t nmemb, 
                       size_t size)
{
    if (nmemb == 0 || size == 0 || alignment == 0) {
        return nullptr;
    }

    if (size > SIZE_MAX / nmemb) {
        return nullptr;
    }

    return arena_alloc(arena, alignment, nmemb * size);
}

bool arena_realloc(Arena *arena, size_t size)
{
    if (size == arena->last_alloc_size) {
        return true;
    }

    M_Pool *const curr_pool = arena->pools[arena->current - 1];

    if (size == 0) {
        /* Delete allocation. */
        curr_pool->offset -= arena->last_alloc_size;
        arena->last_alloc_size = size;
        return true;
    }

    if (size < arena->last_alloc_size) {
        /* Shrink allocation. */
        curr_pool->offset -= arena->last_alloc_size - size;
        arena->last_alloc_size = size;
        return true;
    }

    if (size > (curr_pool->buf_len - curr_pool->offset)) {
        return false;
    }

    /* Expand allocation. */
    curr_pool->offset += size - arena->last_alloc_size;
    arena->last_alloc_size = size;
    return true;
}

Arena *arena_resize(Arena *restrict arena, void *restrict buf, size_t capacity)
{
    if (capacity == 0) {
        if (buf != nullptr) {
            return nullptr;
        }
        capacity = DEFAULT_BUF_CAP;
    }

    if (arena->count >= arena->capacity) {
        arena->capacity *= 2;
        Arena *tmp = realloc(arena,
            sizeof *tmp + (arena->capacity * sizeof arena->pools[0]));

        if (tmp == nullptr) {
            return nullptr;
        }

        arena = tmp;
    }

    M_Pool *const new_pool = pool_new(buf, capacity);

    if (new_pool == nullptr) {
        return nullptr;
    }

    arena->pools[arena->count++] = new_pool;
    ++arena->current;
    return arena;
}

void arena_destroy(Arena *arena)
{
    for (size_t i = 0; i < arena->count; ++i) {
        if (arena->pools[i]->is_heap_alloc) {
            free(arena->pools[i]->buf);
        }
        free(arena->pools[i]);
    }

    free(arena);
}

void arena_reset(Arena *arena)
{
    for (size_t i = 0; i < arena->count; ++i) {
        arena->pools[i]->offset = 0;
    }
    arena->current = 1;
}

#ifdef TEST_MAIN

#include <stdalign.h>

int main(void)
{
    Arena *arena = arena_new(nullptr, 10000);

    if (arena == nullptr) {
        fprintf(stderr, "arena_new() failed to alloc 10000 bytes.\n");
        return EXIT_FAILURE;
    }
    // Allocate memory within the arena
    int *data = arena_alloc(arena, alignof (int), sizeof *data);

    if (data == nullptr) {
        // The backing storage is full. Either add a new pool with
        // arena_resize() or create a new arena.
        fprintf(stderr,
            "arena_alloc() failed to allocate memory for an int.\n");
        return EXIT_FAILURE;
    }
    // Reset the arena and use it like a new one
    arena_reset(arena);

    // Or deallocate all memory associated with it and destroy the arena
    arena_destroy(arena);

    return EXIT_SUCCESS;
}

#endif                          /* TEST_MAIN */

#undef ATTRIB_CONST
#undef ATTRIB_MALLOC
#undef ATTRIB_NONNULL
#undef ATTRIB_NONNULLEX
#undef ATTRIB_INLINE
#undef nullptr
#undef DEFAULT_BUF_CAP
#undef INITIAL_MPOOL_COUNT
#undef D
