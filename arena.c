#include "arena.h"

#include <stdint.h>
#include <stdlib.h>

/* In C2X/C23 or later, nullptr is a keyword. */
/* Patch up C18 (__STDC_VERSION__ == 201710L) and earlier versions.  */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L
    #define nullptr ((void *)0)
#endif

#define ARENA_INITIAL_CAPACITY  10 * (size_t)1024

#define GROW_CAPACITY(capacity, initial) \
    ((capacity) < initial ? initial : (capacity) * 2)

struct arena {
    size_t count;
    size_t capacity;
    uint8_t pool[];
};

Arena *arena_new(void)
{
    Arena *a = calloc(1, sizeof *a + ARENA_INITIAL_CAPACITY * sizeof a->pool[0]);
    a->capacity = ARENA_INITIAL_CAPACITY;
    return a;
}

void *arena_alloc(Arena *arena, size_t alloc_size)
{
    if ((arena->count + alloc_size) >= arena->capacity) {
        arena->capacity =
            GROW_CAPACITY(arena->capacity, ARENA_INITIAL_CAPACITY);
        Arena *a = realloc(arena,
            sizeof *arena + arena->capacity * sizeof arena->pool[0]);

        if (a == nullptr) {
            return nullptr;
        }

        arena = a;
    }

    void *result = arena->pool + arena->count;

    arena->count += alloc_size;
    return result;
}

void arena_destroy(Arena *arena)
{
    free(arena);
}

#ifdef TEST_MAIN

#include <stdio.h>

int main(void)
{
    Arena *const arena = arena_new();

    if (arena == nullptr) {
        fprintf(stderr, "error: arena_new(): failed to allocate memory.\n");
        return EXIT_FAILURE;
    }

    int *a = arena_alloc(arena, sizeof *a);
    int *b = arena_alloc(arena, sizeof *b);
    int *c = arena_alloc(arena, sizeof *c);

    if (a == nullptr || b == nullptr || c == nullptr) {
        arena_destroy(arena);
        fprintf(stderr, "error: arena_alloc(): failed to allocate memory.\n");
        return EXIT_FAILURE;
    }

    *a = 1;
    *b = 2;
    *c = 3;
    
    printf("&a: %p, a: %d\n"
           "&b: %p, b: %d\n"
           "&c: %p, c: %d\n",
           (void *)a,
           *a, 
           (void *)b,
           *b, 
           (void *)c,
           *c);
    arena_destroy(arena);
    return EXIT_SUCCESS;
}

#endif  /* TEST_MAIN */
