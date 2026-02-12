#include <stdio.h>
#include <stdlib.h>

#include "nitro/file.h"
#include "nitro/types.h"

int
nitro_file_read (const char *path, nitro_file *file)
{
  FILE *f = fopen (path, "rb");
  long size;

  file->size = 0;
  file->data = NULL;

  if (f == NULL)
    return -1;

  if (fseek (f, 0, SEEK_END))
    goto fail;

  size = ftell (f);
  if (size < 0)
    goto fail;

  if (fseek (f, 0, SEEK_SET))
    goto fail;

  file->size = size;

  if (file->size > USIZE_MAX - 1)
    goto fail;

  file->data = malloc (file->size + 1);

  if (file->data == NULL)
    goto fail;

  if (!fread (file->data, size, 1, f))
    goto fail;

  file->data[file->size] = '\0';
  fclose (f);

  return 0;

fail:
  free (file->data);
  fclose (f);
  file->size = 0;
  file->data = NULL;
  return -1;
}

void
nitro_file_free (nitro_file *file)
{
  free (file->data);
}
