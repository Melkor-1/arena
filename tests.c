/* NOTE: Use TEST_ASSERT() for unrelated functions. Say malloc() calls, or 
 *       calls to arena_new() when testing arena_alloc(). Else use TEST_CHECK().
 */

/* Compilation fails on MacOS with missing types. This is a kludge at the
 * moment. */
#include <sys/types.h>

#define _POSIX_C_SOURCE 200819L
#define _XOPEN_SOURCE 700

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define HAVE_STDALIGN_H
    #include <stdalign.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "acutest.h"

#include "arena.c"

/* *INDENT-OFF* */
/* In C2X/C23 or later, nullptr is a keyword. */
/* Patch up C18 (__STDC_VERSION__ == 201710L) and earlier versions.  */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L
    #define nullptr ((void *)0)
#endif
/* *INDENT-ON* */

static inline bool is_aligned(const void *ptr, size_t byte_count)
{
    return (uintptr_t) ptr % byte_count == 0;
}

static void test_arena_new(void)
{
    TEST_CHECK(arena_new(stderr, 0) == nullptr);

    Arena *const arena = arena_new(nullptr, 100);

    TEST_CHECK(arena);
    arena_destroy(arena);

    uint8_t *const backing_storage1 = malloc(100 * (size_t) 1024);

    TEST_ASSERT(backing_storage1);

    Arena *const heap_arena = arena_new(backing_storage1, 100 * (size_t) 1024);

    TEST_CHECK(heap_arena);
    arena_destroy(heap_arena);
    free(backing_storage1);

#ifdef HAVE_STDALIGN_H
    static uint8_t alignas (max_align_t) backing_storage2[BUFSIZ];
    Arena *const static_arena =
        arena_new(backing_storage2, sizeof backing_storage2);
    TEST_CHECK(static_arena);
    arena_destroy(static_arena);

    uint8_t alignas (max_align_t) backing_storage3[BUFSIZ];
    Arena *const thread_local_arena =
        arena_new(backing_storage3, sizeof backing_storage3);
    TEST_CHECK(thread_local_arena);
    arena_destroy(thread_local_arena);
#endif
}

static void test_arena_destroy(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    TEST_ASSERT(arena);
    arena_destroy(arena);
}

static void test_arena_reset(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    TEST_ASSERT(arena);
    arena_reset(arena);

    for (size_t i = 0; i < arena->count; ++i) {
        TEST_CHECK(arena->pools[i]->offset == 0);
    }

    TEST_CHECK(arena->current == 1);
    arena_destroy(arena);
}

static void test_arena_alloc(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    TEST_ASSERT(arena);

    TEST_CHECK(arena_alloc(arena, 1, 112) == nullptr);
    TEST_CHECK(arena_alloc(arena, 0, 1) == nullptr);
    TEST_CHECK(arena_alloc(arena, 1, 0) == nullptr);
    TEST_CHECK(arena_alloc(arena, 2, 5) == nullptr);
    TEST_CHECK(arena_alloc(arena, 3, 5) == nullptr);

    TEST_CHECK(arena_alloc(arena, 1, 95));
    uint8_t *const curr_pool = arena->pools[0]->buf;

    TEST_CHECK(curr_pool[96] == 0xA5 && curr_pool[97] == 0xA5
        && curr_pool[98] == 0xA5 && curr_pool[99] == 0xA5);

    arena_reset(arena);

#ifdef HAVE_STDALIGN_H
    const int *const a = arena_alloc(arena, alignof (int), 5 * sizeof *a);
    const double *const b = arena_alloc(arena, alignof (double), 2 * sizeof *b);
    const char *const c = arena_alloc(arena, 1, 10);
    const short *const d = arena_alloc(arena, alignof (short), 5 * sizeof *d);

    TEST_CHECK(a && is_aligned(a, alignof (int)));
    TEST_CHECK(b && is_aligned(b, alignof (double)));
    TEST_CHECK(c && is_aligned(c, 1));
    TEST_CHECK(d && is_aligned(d, alignof (short)));
#endif
    arena_destroy(arena);
}

static void test_arena_resize(void)
{
    Arena *arena = arena_new(nullptr, 1000);

    TEST_ASSERT(arena);

    TEST_CHECK(arena_resize(arena, stderr, 0) == nullptr);

    arena = arena_resize(arena, nullptr, 10000);
    TEST_CHECK(arena);
    TEST_CHECK(arena->current == 2 && arena->count == 2);

    const char *c = arena_alloc(arena, 1, 10000);

    TEST_ASSERT(c);

    arena_reset(arena);
    TEST_CHECK(arena->current == 1 && arena->count == 2);
    arena_destroy(arena);
}

static void test_arena_allocarray(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    TEST_ASSERT(arena);

#ifdef HAVE_STDALIGN_H
    const int *const nums =
        arena_allocarray(arena, alignof (int), 10, sizeof *nums);
    TEST_CHECK(nums);
#endif

    TEST_CHECK(arena_allocarray(arena, 0, 10, 20) == nullptr);
    TEST_CHECK(arena_allocarray(arena, 10, 0, 20) == nullptr);
    TEST_CHECK(arena_allocarray(arena, 10, 20, 0) == nullptr);
    TEST_CHECK(arena_allocarray(arena, 2, 10, SIZE_MAX) == nullptr);

    arena_destroy(arena);
}

static void test_arena_realloc(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    TEST_ASSERT(arena);

    TEST_ASSERT(arena_alloc(arena, 1, 10));
    TEST_CHECK(arena->pools[0]->offset == 10 && arena->last_alloc_size == 10);

    /* Test expansion. */
    TEST_CHECK(arena_realloc(arena, 20));
    TEST_CHECK(arena->pools[0]->offset == 20 && arena->last_alloc_size == 20);

    /* Test shrinking. */
    TEST_CHECK(arena_realloc(arena, 15));
    TEST_CHECK(arena->pools[0]->offset == 15 && arena->last_alloc_size == 15);

    /* Test deletion. */
    TEST_CHECK(arena_realloc(arena, 0));
    TEST_CHECK(arena->pools[0]->offset == 0 && arena->last_alloc_size == 0);
    arena_destroy(arena);
}

static void test_arena_pool_capacity(void)
{
    Arena *const arena = arena_new(nullptr, 100);

    TEST_ASSERT(arena);
    TEST_ASSERT(arena_alloc(arena, 1, 40));
    TEST_CHECK(arena_pool_capacity(arena) == 60);

    TEST_ASSERT(arena_alloc(arena, 1, 49));
    TEST_CHECK(arena_pool_capacity(arena) == 11);

    TEST_ASSERT(arena_alloc(arena, 1, 11));
    TEST_CHECK(arena_pool_capacity(arena) == 0);
    arena_destroy(arena);
}

static void test_arena_allocated_bytes(void)
{
    Arena *arena = arena_new(nullptr, 100);

    TEST_ASSERT(arena);
    arena = arena_resize(arena, nullptr, 10002);
    TEST_ASSERT(arena);
    TEST_CHECK(arena_allocated_bytes(arena) == 10102);
    arena_destroy(arena);
}

static void test_arena_allocated_bytes_including_metadata(void)
{
    Arena *arena = arena_new(nullptr, 100);

    TEST_ASSERT(arena);
    arena = arena_resize(arena, nullptr, 10002);
    TEST_CHECK(arena_allocated_bytes_including_metadata(arena) == 10102
        + offsetof(Arena, pools)
        + sizeof arena->pools[0] * arena->capacity);
    arena_destroy(arena);
}

/* *INDENT-OFF* */
TEST_LIST = {
    { "arena_new", test_arena_new },
    { "arena_destroy", test_arena_destroy },
    { "arena_reset", test_arena_reset },
    { "arena_alloc", test_arena_alloc },
    { "arena_resize", test_arena_resize },
    { "arena_allocarray", test_arena_allocarray },
    { "arena_realloc", test_arena_realloc },
    { "arena_pool_capacity", test_arena_pool_capacity},
    { "arena_allocated_bytes", test_arena_allocated_bytes },
    { "arena_allocated_bytes_including_metadata", test_arena_allocated_bytes_including_metadata },
    { nullptr, nullptr }
};
/* *INDENT-ON* */
