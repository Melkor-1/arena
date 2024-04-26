# An Arena Allocator in C

[![build](https://github.com/Melkor-1/arena/actions/workflows/ci.yml/badge.svg)](https://github.com/Melkor-1/arena/actions/workflows/ci.yml?event=push)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://https://github.com/Melkor-1/arena/edit/main/LICENSE)

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

To build a shared library (`.so`):

```shell
make shared
```

For a static library (`.a`):

```shell
make static
```

To build and run the tests:

```shell
make test
```

The allocator is written in Standard C99 and has been built and tested on these 
platforms:

* Linux 
* Windows
* MacOS
* FreeBSD
* OpenBSD
* NetBSD
* Oracle Solaris

If C11 is available, the tests make use of `stdalign.h`.

## Using with C++:

The instructions to use the library with C++ are present in
[porting_c++.md](porting_c++.md).

