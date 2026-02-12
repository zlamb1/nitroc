#ifndef NITROC_FILE_H
#define NITROC_FILE_H 1

#include "nitro/types.h"

typedef struct
{
  usize size;
  char *data;
} nitro_file;

int nitro_file_read (const char *path, nitro_file *file);
void nitro_file_free (nitro_file *file);

#endif