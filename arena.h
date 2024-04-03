#ifndef ARENA_H
#define ARENA_H 1

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
    #define ATTRIB_NONNULL          __attribute__((nonnull))
#else
    #define ATTRIB_NONNULL          /**/
#endif

typedef struct arena Arena;

Arena *arena_new(void);
void *arena_alloc(Arena *arena, size_t alloc_size) ATTRIB_NONNULL;
void arena_destroy(Arena *arena); 

#endif                          /* ARENA_H */
