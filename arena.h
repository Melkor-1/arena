#ifndef ARENA_H
#define ARENA_H 1

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
    #define ATTRIB_CONST            __attribute__((const))
    #define ATTRIB_MALLOC           __attribute__((malloc))
    #define ATTRIB_NONNULL          __attribute__((nonnull))
    #define ATTRIB_INLINE           __attribute__((always_inline))
#else
    #define ATTRIB_CONST            /**/
    #define ATTRIB_MALLOC           /**/
    #define ATTRIB_NONNULL          /**/
    #define ATTRIB_INLINE           /**/
#endif

/* Bump allocator arena. */
typedef struct arena Arena;

/* Returns a new arena with the specified `capacity`.
 * If `capacity` is 0, a default size of 50 Kib is used.
 * 
 * On allocation failure, returns `nullptr`. */
Arena *arena_new(void *buf, size_t capacity);

/* Destroys the arena, freeing its memory.
 *
 * Any pointer allocated by this arena is invalidated after this call. */
void arena_destroy(Arena *arena) ATTRIB_NONNULL;

/* Resets the arena, invalidating all existing allocations.
 *
 * Whilst existing pointers allocated by this arena are valid after this call 
 * as far as the language is concerned, they should be considered invalid as 
 * using them * would invoke Undefined Behavior. */
void arena_reset(Arena *arena) ATTRIB_NONNULL;

/* Allocates a pointer from the arena.
 *
 * The allocated pointer is suitably aligned for any type that fits into the 
 * requested size or less.
 *
 * If a request can not be entertained, i.e. would overflow, or the arena is full,
 * the function returns `nullptr`. The function also returns a `nullptr` if the 
 * requested size is 0.
 *
 * The allocations are not freed on failure, and remain valid until the arena 
 * is reset or destroyed. 
 */
void *arena_alloc(Arena *arena, size_t size) ATTRIB_MALLOC ATTRIB_NONNULL;

#endif                          /* ARENA_H */
