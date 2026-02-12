#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "nitro/file.h"
#include "nitro/lex.h"

static const char *cmd = "nitroc";

static void
nitro_cmd_error (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  fprintf (stderr, "\033[1m%s: \033[1;31merror: \033[0m", cmd);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
  va_end (args);
  exit (EXIT_FAILURE);
}

static void
nitro_compile_error (const char *filepath, nitro_lex_error error)
{
  const char *msg = error.msg ? error.msg : "out of memory";
  fprintf (stderr,
           "\033[1m%s:%" PRIu32 ":%" PRIu32 ": \033[1;31merror: \033[0m%s\n",
           filepath, error.line, error.col, msg);
  fprintf (stderr, "\033[1m%s: \033[1;31mcompilation failed\033[0m\n", cmd);
  exit (EXIT_FAILURE);
}

int
main (int argc, char *argv[])
{
  nitro_file file;
  nitro_lex_opts opts = { 0 };
  nitro_token_list token_list;

  if (argc && argv[0][0] != '\0')
    cmd = argv[0];

  if (argc < 2)
    nitro_cmd_error ("no input files");

  if (nitro_file_read (argv[1], &file))
    nitro_cmd_error ("failed to read input file '%s'", argv[1]);

  if (nitro_lex (file.size, file.data, &token_list, &opts))
    nitro_compile_error ("", opts.error);

  nitro_file_free (&file);

  nitro_token_list_print (&token_list);
  nitro_token_list_free (&token_list);

  return 0;
}