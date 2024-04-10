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

typedef struct arena {
    size_t count;
    size_t capacity;
    size_t current;
    size_t last_alloc_size;
    M_Pool *pools[];
} Arena;

ATTRIB_INLINE ATTRIB_CONST static inline bool is_power_of_two(uintptr_t x)
{
    return (x & (x - 1)) == 0;
}

ATTRIB_INLINE ATTRIB_CONST static inline bool is_multiple_of(size_t a, size_t b)
{
    return a % b == 0;
}

static M_Pool *pool_new(void *buf, size_t capacity)
{
    if (capacity == 0) {
        if (buf != nullptr) {
            return nullptr;
        }
        capacity = DEFAULT_BUF_CAP;
    }

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
        || alignment == 0
        || (alignment != 1 && !is_power_of_two(alignment))
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
        memset(p + (alignment - remain), 0xA5, alignment - remain);}
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
    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
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

#include <assert.h>
#include <stdalign.h>
#include <stdio.h>

static void test_arena_allocarray(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    assert(arena && "error: arena_new(): failed to allocate memory.\n");

    const int *const nums = arena_allocarray(arena, alignof (int), 10, sizeof *nums);

    assert(nums && "error: arena_allocarray(): failed to allocate memory.\n");
    arena_destroy(arena);
}

static void test_arena_realloc(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    assert(arena && "error: arena_new(): failed to allocate memory.\n");

    assert(arena_alloc(arena, 1, 10));
    assert(arena->pools[0]->offset == 10 && arena->last_alloc_size == 10);

    /* Test expansion. */
    assert(arena_realloc(arena, 20));
    assert(arena->pools[0]->offset == 20 && arena->last_alloc_size == 20);

    /* Test shrinking. */
    assert(arena_realloc(arena, 15));
    assert(arena->pools[0]->offset == 15 && arena->last_alloc_size == 15);

    /* Test deletion. */
    assert(arena_realloc(arena, 0));
    assert(arena->pools[0]->offset == 0 && arena->last_alloc_size == 0);

    arena_destroy(arena);
}

ATTRIB_INLINE ATTRIB_CONST static inline bool is_aligned(const void *ptr,
                                                         size_t      byte_count)
{
    return (uintptr_t) ptr % (byte_count) == 0;
}

static void test_debug_magic_bytes(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    assert(arena && "error: arena_new(): failed to allocate memory.\n");
    assert(arena_alloc(arena, 1, 95));
    uint8_t *const curr_pool = arena->pools[0]->buf;

    assert(curr_pool[96] == 0xA5 && curr_pool[97] == 0xA5
        && curr_pool[98] == 0xA5 && curr_pool[99] == 0xA5);
    arena_destroy(arena);
}

static void test_alignment(void)
{
    Arena *const arena = arena_new(nullptr, 1000);

    assert(arena && "error: arena_new(): failed to allocate memory.\n");

    const int *const a = arena_alloc(arena, alignof (int), 5 * sizeof *a);
    const double *const b = arena_alloc(arena, alignof (double), 2 * sizeof *b);
    const char *const c = arena_alloc(arena, 1, 10);
    const short *const d = arena_alloc(arena, alignof (short), 5 * sizeof *d);

    assert(a && is_aligned(a, alignof (int)));
    assert(b && is_aligned(b, alignof (double)));
    assert(c && is_aligned(c, 1));
    assert(d && is_aligned(d, alignof (short)));
    arena_destroy(arena);
}

static void test_growth(void)
{
    Arena *arena = arena_new(nullptr, 1000);

    assert(arena && "error: arena_new(): failed to allocate memory.\n");
    const char *c = arena_alloc(arena, 1, 10000);

    assert(!c);

    arena = arena_resize(arena, nullptr, 10000);
    assert(arena->current == 2 && arena->count == 2);
    assert(arena && "error: arena_new(): failed to allocate memory.\n");
    c = arena_alloc(arena, 1, 10000);
    assert(c);
    arena_reset(arena);
    assert(arena->current == 1 && arena->count == 2);
    arena_destroy(arena);
}

static void test_failure(void)
{
    assert(arena_new(stderr, 0) == nullptr);

    Arena *const arena = arena_new(nullptr, 100);

    assert(arena && "error: arena_new(): failed to allocate memory.\n");
    assert(arena_alloc(arena, 1, 112) == nullptr);
    assert(arena_alloc(arena, 0, 1) == nullptr);
    assert(arena_alloc(arena, 1, 0) == nullptr);
    assert(arena_alloc(arena, 2, 5) == nullptr);
    assert(arena_alloc(arena, 3, 5) == nullptr);
    arena_reset(arena);
    assert(arena_alloc(arena, 16, 80));
    arena_destroy(arena);
}

static void test_allocation(Arena * arena)
{
    assert(arena && "error: arena_new(): failed to allocate memory.\n");

    char *const c = arena_alloc(arena, 1, 5);
    int *const i = arena_alloc(arena, alignof (int), sizeof *i);
    double *const d = arena_alloc(arena, alignof (double), sizeof *d);

    assert(c && i && d && "error: arena_alloc(): failed to allocate memory.\n");

    *c = 'A';
    *i = 1;
    *d = 20103.212;

    printf("&c (char *): %p, c: %c\n"
        "&i (int *): %p, i: %d\n"
        "&d (double *): %p, d: %lf\n",
        (void *) c, *c, (void *) i, *i, (void *) d, *d);
    arena_destroy(arena);
}

static void test_client_static_arena(void)
{
    static uint8_t alignas (max_align_t) static_pool[BUFSIZ];
    Arena *const static_arena = arena_new(static_pool, sizeof static_pool);

    puts("---- Using a statically-allocated arena ----");
    test_allocation(static_arena);
}

static void test_client_automatic_arena(void)
{
    uint8_t alignas (max_align_t) thread_local_pool[BUFSIZ];
    Arena *const thread_local_arena =
        arena_new(thread_local_pool, sizeof thread_local_pool);

    puts("---- Using an automatically-allocated arena ----");
    test_allocation(thread_local_arena);
}

static void test_client_dynamic_arena(void)
{
    uint8_t *const client_heap_pool = malloc(100 * (size_t) 1024);

    assert(client_heap_pool && "error: failed to allocate client_heap_pool.\n");

    Arena *const client_heap_arena =
        arena_new(client_heap_pool, 100 * (size_t) 1024);

    puts("---- Using a dynamically-allocated arena ----");
    test_allocation(client_heap_arena);
    free(client_heap_pool);
}

static void test_lib_dynamic_arena(void)
{
    Arena *const lib_arena = arena_new(nullptr, 100);

    puts("---- Using the library's internal arena ----");
    test_allocation(lib_arena);
}

int main(void)
{
    test_lib_dynamic_arena();
    test_client_dynamic_arena();
    test_client_automatic_arena();
    test_client_static_arena();
    test_arena_allocarray();
    test_arena_realloc();
    test_failure();
    test_alignment();
    test_growth();
    D(test_debug_magic_bytes());
    return EXIT_SUCCESS;
}

#undef ATTRIB_CONST
#undef ATTRIB_MALLOC
#undef ATTRIB_NONNULL
#undef ATTRIB_NONNULLEX
#undef ATTRIB_INLINE
#undef nullptr
#undef DEFAULT_BUF_CAP
#undef INITIAL_MPOOL_COUNT
#undef DEBUG
#undef D

#endif                          /* TEST_MAIN */
