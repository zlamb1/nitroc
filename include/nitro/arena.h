#ifndef NITROC_ARENA_H
#define NITROC_ARENA_H 1

#include "nitro/types.h"

#ifndef NITRO_ARENA_ALIGN
#define NITRO_ARENA_ALIGN _Alignof (max_align_t)
#endif

#ifndef nitro_aligned_alloc
#define nitro_aligned_alloc aligned_alloc
#endif

typedef struct
{
  u64 align, size, prev_size;
  void *base, *cursor, *prev;
} nitro_arena;

int nitro_arena_acquire (nitro_arena *arena, u64 init_size);
int nitro_arena_acquire_ext (nitro_arena *arena, u64 align, u64 init_size);

void *nitro_arena_alloc (nitro_arena *arena, u64 size);
void *nitro_arena_mem_align (nitro_arena *arena, u64 align, u64 size);

void nitro_arena_free (nitro_arena *arena, void *p);
void nitro_arena_release (nitro_arena *arena);

#endif