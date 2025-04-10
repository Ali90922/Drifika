Oracle Thread Pool Example
==========================

This package contains an example of some client code that uses the [Oracle Thread
Pool Implementation].

The code in `thr_pool.h` and `thr_pool.c` has been copied from Oracle's example
with the following modifications:

* `uint_t` -> `uint16_t` in all places and adding `#include <stdint.h>`
* Creating a temporary `pthread_t` in `create_worker` to pass to
  `pthread_create` (`clang` warns that the first argument should not be `NULL`).
* Signature of some functions changed to match expected signature for
  `pthread_create` (e.g., `worker_cleanup` changed to accept `void *` instead of
  `thr_pool_t *`).
* `timestruc_t` -> `struct timespec` in one place (line 151).
* Added `wrap_unlock_mutex` so that we can have the correct signature for
  `pthread_cleanup_push` on line 362.

[Oracle Thread Pool Implementation]:
https://docs.oracle.com/cd/E19253-01/816-5137/ggedn/index.html

Building
--------

This example uses `make`, you can run

```bash
make
```

in the current directory to build the example.

Running
-------

[Building] produces two binary files: `pool.o` and `thread_madness`. You can run
the example by running

```bash
./thread_madness
```

in the current directory. It doesn't really do very much. In fact, it's pretty
much the same result as what you would expect launching $N$ threads and having
them all `++` a variable.

Note: the thing we're adding to is a file-scope variable, the output is expected
to change each time because we're running code that contains an unprotected
critical section.

WARNING
-------

This code is for demonstration purposes only, you really shouldn't use it for
anything meaningful.
