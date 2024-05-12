## Guidelines for compiling with C++:

### Things to be changed:

* The `restrict` keyword. Though `restrict` is supported by many C++ compilers, 
  it is not part of the C++ standard yet, and has to be removed (unless using 
  compiler extensions).
* `nullptr` keyword. To use `nullptr` portably across all C and C++ versions, 
  the check for it should be replaced to this:

  ```c
  /* In C2X/C23 or later, nullptr is a keyword. */
  /* Patch up C18 (__STDC_VERSION__ == 201710L) and earlier versions, and the
   * versions preceding C++11 as well.  */
  #if (!defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L) || (defined(__cplusplus) && __cplusplus <= 201103L)
      #define nullptr ((void *)0)
  #endif
  ```

* Designated initializers. C++ does not support designated initializers before
  C++20. Either remove them, or compile with std=c++20 in the Makefile.

* Conversions from `void *` to any other pointer type. This is mostly required
  in the calls to `malloc()` and family, and probably some more places. The fix
  is to use casts.

* Flexible array members in `struct Arena` and `struct M_Pool`. Flexible array members 
  are illegal in C++. The fix is to allocate memory separately for them.

* `extern "C"`. In the arena.h header, add:

  ```c
  #ifdef __cplusplus
      extern "C" {
  #endif

  // contents go here

  #ifdef __cplusplus
      }
  #endif
  ```

