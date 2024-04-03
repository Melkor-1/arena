# An Arena Allocator in C

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://https://github.com/Melkor-1/lexer-cpp/edit/main/LICENSE)

An arena allocator is a memory allocation technique used in computer programming where memory is allocated from 
a pre-allocated block (arena) rather than individually requesting memory from the operating system. 

Once allocated, memory within the arena typically remains allocated until the entire arena is deallocated,
simplifying memory management and reducing the risk of memory leaks.

## Working:

The API is straightforward: begin with `arena_new()` and end with `arena_destroy()`. Memory allocations are 
made using `arena_alloc()`, without the need for manual deallocation or tracking. The arena dynamically resizes 
as needed.

An example follows:

```c
#include <stdio.h>
#include <stdlib.h>

#include "arena.h"

int main(void) {
    // Create a new arena
    Arena *arena = arena_new();

    // Allocate memory within the arena
    int *data = arena_alloc(arena, sizeof *data);

    // Deallocate memory and destroy the arena
    arena_destroy(arena);

    return EXIT_SUCCESS;
}
```

## Building:

The whole implementation is about 60 lines of code (besides the test run).

To build a sample program, clone the repository and run:

```shell
cd arena
make test
./arena
```
