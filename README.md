# An Arena Allocator in C

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://https://github.com/Melkor-1/lexer-cpp/edit/main/LICENSE)

An arena allocator is a memory allocation technique used in computer programming where memory is allocated from 
a pre-allocated block (arena) rather than individually requesting memory from the operating system. 

Once allocated, memory within the arena typically remains allocated until the entire arena is deallocated,
simplifying memory management and reducing the risk of memory leaks.

## Working:

The API is straightforward: begin with `arena_new()` and end with `arena_destroy()`. Memory allocations are 
made using `arena_alloc()`, without the need for manual deallocation or tracking. The arena can be resized
with `arena_resize()`.

The arena can use a buffer passed by the client as backing storage, or allocate a
buffer of its own. An example follows:

```c
#include <stdio.h>
#include <stdlib.h>

#include "arena.h"

int main(void) 
{
    Arena *arena = arena_new(NULL, 10000);

    if (arena == NULL) {
        // Memory allocated failed
    }

    // Allocate memory within the arena
    int *data = arena_alloc(arena, alignof(int), sizeof *data);

    if (data == NULL) {
        // The backing storage is full. Either add a new pool with
        // arena_resize() or create a new arena.
    }

    // Reset the arena and use it like a new one
    arena_reset(arena);

    // Or deallocate all memory associated with it and destroy the arena
    arena_destroy(arena);

    return EXIT_SUCCESS;
}
```

The rest of the API, and its documentation, can be found in `arena.h`.

## Building:

The whole implementation is about 230 lines of code (besides the test run).

To build a sample program, clone the repository and run:

```shell
cd arena
make 
./arena
```

For a debug build:

```shell
make debug
```

