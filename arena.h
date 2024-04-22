#ifndef ARENA_H
#define ARENA_H 1

/* *INDENT-OFF* */
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
    #define ATTRIB_CONST            __attribute__((const))
    #define ATTRIB_PURE             __attribute__((pure))
    #define ATTRIB_MALLOC           __attribute__((malloc))
    #define ATTRIB_NONNULL          __attribute__((nonnull))
    #define ATTRIB_NONNULLEX(...)   __attribute__((nonnull(__VA_ARGS__)))
    #define ATTRIB_INLINE           __attribute__((always_inline))
#else
    #define ATTRIB_CONST            /**/
    #define ATTRIB_MALLOC           /**/
    #define ATTRIB_NONNULL          /**/
    #define ATTRIB_NONNULLEX(...)   /**/
    #define ATTRIB_INLINE           /**/
#endif
/* *INDENT-ON* */

#define DEFAULT_BUF_CAP     256 * (size_t)1024

#include <stdbool.h>
#include <stddef.h>

/* Bump allocator arena. */
typedef struct arena Arena;

/* Returns a new arena with the specified `capacity`.
 * If `capacity` is 0, a default size of `DEFAULT_BUF_CAP` is used.
 * 
 * On allocation failure, or if `buf` is a non-null pointer and `capacity` is 0,
 * returns `nullptr`. 
 *
 * Passing a `buf` that is smaller than the specified `capacity` would invoke
 * undefined behavior. */
Arena *arena_new(void *buf, size_t capacity);

/* Destroys `arena`, freeing all the memory associated with it.
 *
 * Any pointer allocated by this arena is invalidated after this call. */
void arena_destroy(Arena *arena) ATTRIB_NONNULL;

/* Resets `arena`, invalidating all existing allocations.
 *
 * Whilst existing pointers allocated by this arena are valid after this call 
 * as far as the language is concerned, they should be considered invalid as 
 * using them * would invoke Undefined Behavior. */
void arena_reset(Arena *arena) ATTRIB_NONNULL;

/* Allocates a pointer from `arena`.
 *
 * The allocated pointer is at least aligned to `alignment`.
 *
 * `alignment` must be a power of 2.
 * 
 * `size` must be a multiple of `alignment`.
 *
 * If a request can not be entertained, i.e. would overflow, or `arena` is full,
 * the function returns `nullptr`. The function also returns a `nullptr` if the 
 * requested `size` or `alignment` is 0 or if `alignment` is not a power of 2, 
 * or if `size` is not a multiple of `alignment`.
 *
 * Any allocations made prior to this call are not freed on failure, and remain 
 * valid until the arena is either reset or destroyed.
 */
void *arena_alloc(Arena *arena, size_t alignment, size_t size)
    ATTRIB_MALLOC ATTRIB_NONNULL;

/* Adds a new memory pool to the existing arena `arena`.
 * If `capacity` is 0, a default size of `DEFAULT_BUF_CAP` is used.
 *
 * On allocation failure, or if `buf` is a non-null pointer and `capacity` is 0,
 * returns `nullptr`. 
 *
 * Any allocations made prior to this call are not freed on failure, and remain 
 * valid until the arena is either reset or destroyed.

 * Passing a `buf` that is smaller than the specified `capacity`, or passing an 
 * `arena` that was not returned by `arena_new()` would invoke Undefined Behavior. */
Arena *arena_resize(Arena *restrict arena, void *restrict buf, size_t capacity) 
    ATTRIB_NONNULLEX(1);

/* Allocates a pointer from `arena` large enough for an array of `nmemb` 
 * elements, each of which is `size` bytes.  It is equivalent to the call:
 *
 *              arena_alloc(arena, alignment, nmemb * size);
 * 
 * However, unlike that `arena_alloc()` call, `arena_allocarray()` fails safely 
 * in the case where the multiplication would overflow. If such an overflow 
 * occurs, `arena_allocarray()` returns `nullptr`. 
 *
 * Has the same requirements for `size` and `alignment` as `arena_alloc()`, and 
 * returns `nullptr` for all the cases `arena_alloc()` does.
 *
 * Any allocations made prior to this call are not freed on failure, and remain 
 * valid until the arena is either reset or destroyed.
 */
void *arena_allocarray(Arena *arena, 
                       size_t alignment, 
                       size_t nmemb,
                       size_t size) ATTRIB_MALLOC ATTRIB_NONNULL;

/* Extends the last allocation in place.
 *
 * If `size` is 0, the last allocation is deleted. Else if it is less than the
 * last allocated size, it is shrinked to `size`. Else, it is expanded to `size`
 * bytes.
 *
 * Returns `false` if the request can not be entertained, i.e out of memory.
 * Else it returns `true`.
 */
bool arena_realloc(Arena *arena, size_t size) ATTRIB_NONNULL;

/* Gets the remaining capacity in the current pool (in bytes). */
size_t arena_pool_capacity(Arena *arena) ATTRIB_PURE;

/* Calculates the number of bytes currently allocated across all chunks in `arena`.
 *
 * If you allocate types of different alignments or types with larger-than-typical 
 * alignment in the same arena, some padding bytes might get allocated in the 
 * arena. Those padding bytes will add to this function's resulting sum. The 
 * allocated bytes do not include the size of this arena's metadata. */
size_t arena_allocated_bytes(Arena *arena) ATTRIB_PURE;

/* Calculates the number of bytes requested from the C allocator for this arena. 
 * This number is equal to the allocated_bytes() plus the size of the arena 
 * metadata. */
size_t arena_allocated_bytes_including_metadata(Arena *arena) ATTRIB_PURE;

#endif                          /* ARENA_H */
