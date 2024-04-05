#include "arena.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* In C2X/C23 or later, nullptr is a keyword. */
/* Patch up C18 (__STDC_VERSION__ == 201710L) and earlier versions.  */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L
#define nullptr ((void *)0)
#endif

#ifdef DEBUG
#include <string.h>
#define D(x) x
#else
#define D(x) (void) 0
#endif

#define ARENA_INITIAL_CAPACITY  (size_t)1024 * 50

struct arena {
    size_t count;
    size_t capacity;
    bool is_heap_alloc;
    uint8_t *pool;
};

ATTRIB_CONST ATTRIB_INLINE static inline size_t max(size_t a, size_t b)
{
    return a > b ? a : b;
}

ATTRIB_CONST ATTRIB_INLINE static inline size_t max_alignof(void)
{
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#include <stdalign.h>
    return alignof (max_align_t);
#else
    /* For C99, see: https://stackoverflow.com/q/38271072/2001754://stackoverflow.com/q/38271072/20017547 */
    return max(max(max(sizeof (intmax_t), sizeof (long double)), sizeof (void)),
        sizeof (void (*)()));
#endif
}

Arena *arena_new(void *buf, size_t capacity)
{
    if (capacity == 0) {
        if (buf != nullptr) {
            return nullptr;
        }
        capacity = ARENA_INITIAL_CAPACITY;
    }

    Arena *a = calloc(1, sizeof *a);

    if (a) {
        *a = (Arena) {
        .pool = buf ? buf : calloc(1, capacity),.capacity =
                capacity,.is_heap_alloc = buf == nullptr,};

        if (a->pool == nullptr) {
            free(a);
            return nullptr;
        }
    }
    return a;
}

void *arena_alloc(Arena *arena, size_t size)
{
    if (size == 0) {
        return nullptr;
    }

    const size_t alignment = max_alignof();
    const size_t remain = size % alignment;

    if (remain) {
        if ((alignment - remain) > SIZE_MAX - size) {
            return nullptr;
        }
        size += alignment - remain;
    }

    if (size > arena->capacity - arena->count) {
        return nullptr;
    }

    /* Set the bytes immediately before a user block, and the bytes immediately 
     * following such a block to non-zero. The intent is to trigger OBOB failures 
     * to inappropiate app use of strlen()/strnlen(), which keep forging ahead 
     * till encountering ascii NUL. */
    /* 0xA5 is used in FreeBSD's PHK malloc for debugging purposes. */
    D(if (remain) {
        memset(arena->pool + arena->count + (alignment - remain), 0xA5,
                alignment - remain);}
    );
    void *const p = arena->pool + arena->count;

    arena->count += size;
    D(memset(arena->pool + arena->count, 0xA5, arena->capacity - arena->count));
    return p;
}

void arena_destroy(Arena *arena)
{
    if (arena->is_heap_alloc) {
        free(arena->pool);
    }
    free(arena);
}

void arena_reset(Arena *arena)
{
    /* We can reuse the same pool for subsequent calls. */
    arena->count = 0;
}

#undef ARENA_INITIAL_CAPACITY
#undef ATTRIB_CONST
#undef ATTRIB_MALLOC
#undef ATTRIB_NONNULL
#undef ATTRIB_INLINE

#ifdef TEST_MAIN

#include <assert.h>
#include <stdio.h>

static void test_failure(void)
{
    assert(arena_new(stderr, 0) == nullptr);

    Arena *const arena = arena_new(nullptr, 100);

    assert(arena && "error: arena_new(): failed to allocate memory.\n");

    D(
        if (max_alignof() == 16) {
            assert(arena_alloc(arena, 96));
            assert(arena->pool[96] == 0xA5 && arena->pool[97] == 0xA5
                && arena->pool[98] == 0xA5 && arena->pool[99] == 0xA5);
        }
    );

    assert(arena_alloc(arena, 112) == nullptr);

    arena_reset(arena);

    assert(arena_alloc(arena, 99) == nullptr);
    assert(arena_alloc(arena, 80));

    arena_destroy(arena);
}

static void test_allocation(Arena *arena)
{
    assert(arena && "error: arena_new(): failed to allocate memory.\n");

    char *c = arena_alloc(arena, sizeof *c);
    int *i = arena_alloc(arena, sizeof *i);
    double *d = arena_alloc(arena, sizeof *d);
    FILE **fp = arena_alloc(arena, sizeof *fp);
    
    assert(c && i && d && fp && "error: arena_alloc(): failed to allocate memory.\n");

    *c = 'A';
    *i = 1;
    *d = 20103.212;
    *fp = stderr;

    printf("&c (char *): %p, c: %c\n"
        "&i (int *): %p, i: %d\n"
        "&d (double *): %p, d: %lf\n"
        "&fp (FILE *) %p\n\n",
        (void *) c, *c, (void *) i, *i, (void *) d, *d, (void *) fp);
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
    uint8_t *client_heap_pool = malloc(100 * (size_t) 1024);

    assert(client_heap_pool && "error: failed to allocate client_heap_pool.\n");

    Arena *client_heap_arena = arena_new(client_heap_pool, 100 * (size_t) 1024);

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
    test_failure();
    return EXIT_SUCCESS;
}

#endif                          /* TEST_MAIN */
