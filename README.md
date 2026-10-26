# Malloc

Part of 42's post common core UNIX projects.

The goal of this project is to recreate a dynamic memory allocator implementing:

- `malloc()`
- `realloc()`
- `free()`
- `show_alloc_mem()` to print allocations and their size

It also meets the following requirements:

- All pointers are aligned properly according to `max_align_t`
- Block headers and payload sizes are also aligned accordingly
- Minimizes the amount of calls to `mmap()` or `munmap()`

## Architecture

Allocations are split into three categories/sizes:

- TINY
- SMALL
- LARGE

TINY and SMALL allocations are served from preallocated `mmap()` zones when possible.

LARGE allocations use their own dedicated `mmap()` region and are directly `munmap()`'d when freed.

Each memory block contains a header used to track:

- physical neighbours
- freelist membership
- returnable zone membership
- block size
- zone type
- mapping boundaries

Free blocks are stored in size-based freelists and reused by future allocations.

Adjacent free blocks belonging to the same mapped zone are merged to reduce fragmentation.

## Bonus part

As part of 42's subject, all bonuses were implemented, features such as :

- Handling malloc related env variables
- Freed memory defragmentation
- Thread-safe multithreading capability
- Extended debug print tools

## Building and usage

Build the shared library:

```bash
git clone https://github.com/0yech/malloc.git
cd malloc
make
```

This produces a platform-specific library such as `libft_malloc_x86_64_Linux.so` and symlink `libft_malloc.so`

Replace other implementations of `malloc()` in a program through `LD_PRELOAD`:

```bash
LD_PRELOAD=./libft_malloc.so ./program
    example
LD_PRELOAD=./libft_malloc.so /bin/ls
```

## Debug functions

The project provides:

```c
void show_alloc_mem(void);
void show_alloc_mem_ex(void);
```

`show_alloc_mem()` displays the currently allocated blocks.

`show_alloc_mem_ex()` provides a more detailed view of the allocator state, including additional block information and memory contents useful during debugging.

Debug output is implemented using `write()` calls rather than `printf()`to avoid recursive calls to `malloc()`.

## Env variables
**`MALLOC_DEBUG`** and **`MALLOC_SCRIBBLE`** are available as env variables.

```
MALLOC_DEBUG=1
```
Prints out a log whenever the three functions are called. `malloc(55) - 0x...`

```
MALLOC_SCRIBBLE=1
```
Fills allocated and freed memory with recognizable patterns. Fresh allocs are filled with `0xAA` and freed allocs with `0xDD`

## Extras

### Returnable zones

Fully free TINY and SMALL zones can be kept temporarily in a returnable list for reuse.

Keeping a limited number of free zones allows future allocations to reuse already mapped memory instead of immediately calling `mmap()` again.

When too many fully free zones accumulate, the excess zones are returned to the operating system with `munmap()`.

LARGE allocations are always unmapped directly when freed.

This implementation of malloc was somewhat heavily slowed down by the subject requirements. Returnable lists should've been a major upgrade if users had to trim their zones manually. But since the project must not leak only using `free()`, its usefulness and overall performance was hurt.