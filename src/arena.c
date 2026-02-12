#include <stddef.h>
#include <stdlib.h>

#include "nitro/arena.h"
#include "nitro/types.h"

#define ALIGN_UP(X, A) (((X) + (A - 1)) & ~(A - 1))

static inline void
arena_zero (nitro_arena *arena)
{
  arena->size = arena->prev_size = 0;
  arena->base = arena->cursor = arena->prev = NULL;
}

int
nitro_arena_acquire (nitro_arena *arena, u64 init_size)
{
  return nitro_arena_acquire_ext (arena, init_size, NITRO_ARENA_ALIGN);
}

int
nitro_arena_acquire_ext (nitro_arena *arena, u64 align, u64 init_size)
{
  if (align & (align - 1) || init_size > U64_MAX - (align - 1))
    {
      arena_zero (arena);
      return -1;
    }

  init_size = ALIGN_UP (init_size, align);

  arena->align = align;
  arena->size = init_size;
  arena->base = nitro_aligned_alloc (align, init_size);
  arena->cursor = arena->base;
  arena->prev_size = 0;
  arena->prev = NULL;

  if (arena->base == NULL)
    {
      arena_zero (arena);
      return -1;
    }

  return 0;
}

void *
nitro_arena_alloc (nitro_arena *arena, u64 size)
{
  void *p;

  if (size > U64_MAX - (arena->align - 1))
    return NULL;

  size = ALIGN_UP (size, arena->align);

  if (size > arena->size)
    return NULL;

  p = arena->cursor;

  arena->size -= size;
  arena->cursor = (char *) arena->cursor + size;
  arena->prev_size = size;
  arena->prev = p;

  return p;
}

void *
nitro_arena_mem_align (nitro_arena *arena, u64 align, u64 size)
{
  uintptr_t addr, diff;
  void *p;

  if (align & (align - 1))
    return NULL;

  if (align < arena->align)
    align = arena->align;

  if (size > U64_MAX - (arena->align - 1))
    return NULL;

  size = ALIGN_UP (size, arena->align);

  addr = (uintptr_t) arena->cursor;
  if (addr > U64_MAX - (align - 1))
    return NULL;

  addr = ALIGN_UP (addr, align);
  diff = addr - (uintptr_t) arena->cursor;

  if (size > U64_MAX - diff)
    return NULL;

  size += diff;
  if (size > arena->size)
    return NULL;

  p = (void *) addr;

  arena->size -= size;
  arena->cursor = (char *) arena->cursor + size;
  arena->prev = p;
  arena->prev_size = size;

  return p;
}

void
nitro_arena_free (nitro_arena *arena, void *p)
{
  if (p == NULL || arena->prev != p)
    return;

  arena->size += arena->prev_size;
  arena->cursor = (char *) arena->cursor - arena->prev_size;

  arena->prev_size = 0;
  arena->prev = NULL;
}

void
nitro_arena_release (nitro_arena *arena)
{
  free (arena->base);
  arena_zero (arena);
}