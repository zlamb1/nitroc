#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nitro/arena.h"
#include "nitro/lex.h"

#define TOKEN_ALLOC(TOKEN)                                                    \
  do                                                                          \
    {                                                                         \
      TOKEN = nitro_arena_alloc (&token_list->token_arena,                    \
                                 sizeof (nitro_token));                       \
                                                                              \
      if (TOKEN == NULL)                                                      \
        {                                                                     \
          tmperror = strdup ("out of memory");                                \
          goto fail;                                                          \
        }                                                                     \
                                                                              \
      memset (TOKEN, 0, sizeof (nitro_token));                                \
      TOKEN->line = line;                                                     \
      TOKEN->col = col;                                                       \
      ++token_list->len;                                                      \
    }                                                                         \
  while (0)

#define TOKEN1_IF(CH1, KIND)                                                  \
  if (ch == (CH1))                                                            \
    {                                                                         \
      TOKEN_ALLOC (cur_token);                                                \
      cur_token->kind = (KIND);                                               \
      cur_token->flags = NITRO_TOKEN_FLAG_SSO;                                \
      cur_token->sso.len = 1;                                                 \
      cur_token->sso.d[0] = (CH1);                                            \
      ++col;                                                                  \
      cur_token = NULL;                                                       \
      continue;                                                               \
    }

#define TOKEN2_IF(CH1, CH2, KIND)                                             \
  if (ch == (CH1) && str[0] == (CH2))                                         \
    {                                                                         \
      TOKEN_ALLOC (cur_token);                                                \
      cur_token->kind = (KIND);                                               \
      cur_token->flags = NITRO_TOKEN_FLAG_SSO;                                \
      cur_token->sso.len = 2;                                                 \
      cur_token->sso.d[0] = (CH1);                                            \
      cur_token->sso.d[1] = (CH2);                                            \
      col += 2;                                                               \
      (--len, ++str);                                                         \
      cur_token = NULL;                                                       \
      continue;                                                               \
    }

const char *nitro_keyword_table[NITRO_KW_COUNT] = {
  [NITRO_KW_BOOL] = "bool",     [NITRO_KW_FALSE] = "false",
  [NITRO_KW_TRUE] = "true",     [NITRO_KW_VOID] = "void",
  [NITRO_KW_NULL] = "null",     [NITRO_KW_U8] = "u8",
  [NITRO_KW_U16] = "u16",       [NITRO_KW_U32] = "u32",
  [NITRO_KW_U64] = "u64",       [NITRO_KW_I8] = "i8",
  [NITRO_KW_I16] = "i16",       [NITRO_KW_I32] = "i32",
  [NITRO_KW_I64] = "i64",       [NITRO_KW_F32] = "f32",
  [NITRO_KW_F64] = "f64",       [NITRO_KW_IF] = "if",
  [NITRO_KW_FOR] = "for",       [NITRO_KW_WHILE] = "while",
  [NITRO_KW_CONT] = "continue", [NITRO_KW_BRK] = "break",
  [NITRO_KW_GOTO] = "goto",     [NITRO_KW_ENUM] = "enum",
  [NITRO_KW_STRUCT] = "struct", [NITRO_KW_UNION] = "union",
  [NITRO_KW_FN] = "fn",         [NITRO_KW_PACKED] = "packed",
  [NITRO_KW_PUB] = "pub",       [NITRO_KW_DEFER] = "defer",
};

static inline int
nitro_is_ident_first (char ch)
{
  return ch == '_' || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static inline int
nitro_is_ident (char ch)
{
  return nitro_is_ident_first (ch) || (ch >= '0' && ch <= '9');
}

static int
nitro_ident_token_finalize (nitro_token *token, const char *str, usize cur_len,
                            nitro_arena *string_arena, char **tmperror)
{
  for (int i = NITRO_KW_RESERVED + 1; i < NITRO_KW_COUNT; ++i)
    {
      if (!strncmp (str - cur_len, nitro_keyword_table[i], cur_len))
        {
          token->kind = NITRO_TOKEN_KEYWORD;
          token->flags |= NITRO_TOKEN_FLAG_SSO;
          token->kw = i;
          token->sso.len = strlen (nitro_keyword_table[i]);
          memcpy (token->sso.d, nitro_keyword_table[i], token->sso.len);
          goto done;
        }
    }

  if (cur_len > NITRO_TOKEN_SSO_MAX_LEN)
    {
      token->flags |= NITRO_TOKEN_FLAG_STR;
      token->s.len = cur_len;
      token->s.d = nitro_arena_alloc (string_arena, cur_len);

      if (token->s.d == NULL)
        {
          *tmperror = strdup ("out of memory");
          return -1;
        }

      memcpy (token->s.d, str - cur_len, cur_len);
    }
  else
    {
      token->flags |= NITRO_TOKEN_FLAG_SSO;
      token->sso.len = cur_len;
      memcpy (token->sso.d, str - cur_len, cur_len);
    }

done:
  return 0;
}

int
nitro_lex (usize len, const char *str, nitro_token_list *token_list,
           nitro_lex_opts *_opts)
{
  char *tmperror = NULL, **error;
#define STATE_NONE  0
#define STATE_IDENT 1
  int state = STATE_NONE;
#define COL 1
  usize line = 1, col = COL, cur_len;
  nitro_lex_opts opts;
  nitro_token *cur_token = NULL;

  if (_opts != NULL)
    {
      opts = *_opts;
      error = &_opts->error.msg;
    }
  else
    {
      opts = (nitro_lex_opts) { 0 };
      error = NULL;
    }

  if (!opts.token_arena_size)
    opts.token_arena_size = 1024 * 64;

  if (!opts.strings_arena_size)
    opts.strings_arena_size = 1024 * 64;

  *token_list = (nitro_token_list) { 0 };

  if (nitro_arena_acquire_ext (&token_list->token_arena,
                               _Alignof (nitro_token), opts.token_arena_size)
      || nitro_arena_acquire_ext (&token_list->string_arena, 1,
                                  opts.strings_arena_size))
    {
      tmperror = strdup ("out of memory");
      goto fail;
    }

  token_list->tokens = token_list->token_arena.base;

  while (len)
    {
      --len;
      char ch = *str++;

      if (state == STATE_NONE)
        {
          if (ch == ' ' || ch == '\t')
            {
              col += ch == ' ' ? 1 : 4;
              continue;
            }
          else if (ch == '\n')
            {
              ++line;
              col = COL;
              continue;
            }
          else if (ch == '\r')
            {
              ++line;
              col = COL;
              if (len && str[0] == '\n')
                (--len, ++str);
              continue;
            }

          if (nitro_is_ident_first (ch))
            {
              TOKEN_ALLOC (cur_token);
              cur_token->kind = NITRO_TOKEN_IDENT;
              ++col;
              state = STATE_IDENT;
              cur_len = 1;
              continue;
            }

          if (!len)
            goto onetok;

          TOKEN2_IF ('-', '>', NITRO_TOKEN_ARROW);
          TOKEN2_IF ('+', '=', NITRO_TOKEN_PLUS_EQ);
          TOKEN2_IF ('-', '=', NITRO_TOKEN_MINUS_EQ);
          TOKEN2_IF ('*', '=', NITRO_TOKEN_STAR_EQ);
          TOKEN2_IF ('/', '=', NITRO_TOKEN_DIVIDE_EQ);
          TOKEN2_IF ('%', '=', NITRO_TOKEN_MOD_EQ);
          TOKEN2_IF ('=', '=', NITRO_TOKEN_EQ_EQ);
          TOKEN2_IF ('!', '=', NITRO_TOKEN_NOT_EQ);
          TOKEN2_IF ('>', '=', NITRO_TOKEN_GR_EQ);
          TOKEN2_IF ('<', '=', NITRO_TOKEN_LS_EQ);
          TOKEN2_IF ('&', '=', NITRO_TOKEN_BIT_AND_EQ);
          TOKEN2_IF ('|', '=', NITRO_TOKEN_BIT_OR_EQ);
          TOKEN2_IF ('^', '=', NITRO_TOKEN_BIT_XOR_EQ);
          TOKEN2_IF ('&', '&', NITRO_TOKEN_AND);
          TOKEN2_IF ('|', '|', NITRO_TOKEN_OR);

        onetok:
          TOKEN1_IF ('(', NITRO_TOKEN_LPAREN);
          TOKEN1_IF (')', NITRO_TOKEN_RPAREN);
          TOKEN1_IF ('[', NITRO_TOKEN_LBRACK);
          TOKEN1_IF (']', NITRO_TOKEN_RBRACK);
          TOKEN1_IF ('{', NITRO_TOKEN_LBRACE);
          TOKEN1_IF ('}', NITRO_TOKEN_RBRACE);
          TOKEN1_IF (',', NITRO_TOKEN_COMMA);
          TOKEN1_IF ('.', NITRO_TOKEN_PERIOD);
          TOKEN1_IF ('!', NITRO_TOKEN_EXCLAIM);
          TOKEN1_IF (';', NITRO_TOKEN_SEMICOLON);
          TOKEN1_IF ('+', NITRO_TOKEN_PLUS);
          TOKEN1_IF ('-', NITRO_TOKEN_MINUS);
          TOKEN1_IF ('*', NITRO_TOKEN_STAR);
          TOKEN1_IF ('/', NITRO_TOKEN_DIVIDE);
          TOKEN1_IF ('%', NITRO_TOKEN_MOD);
          TOKEN1_IF ('=', NITRO_TOKEN_EQ);
          TOKEN1_IF ('>', NITRO_TOKEN_GR);
          TOKEN1_IF ('<', NITRO_TOKEN_LS);
          TOKEN1_IF ('~', NITRO_TOKEN_BIT_COMPL);
          TOKEN1_IF ('&', NITRO_TOKEN_BIT_AND);
          TOKEN1_IF ('|', NITRO_TOKEN_BIT_OR);
          TOKEN1_IF ('^', NITRO_TOKEN_BIT_XOR);
          TOKEN1_IF ('@', NITRO_TOKEN_BUILTIN);

          if (asprintf (&tmperror, "unknown token '%c'", ch) < 0)
            tmperror = NULL;

          goto fail;
        }

      if (state == STATE_IDENT)
        {
          if (!nitro_is_ident (ch))
            {
              (++len, --str);

              if (nitro_ident_token_finalize (cur_token, str, cur_len,
                                              &token_list->string_arena,
                                              &tmperror))
                goto fail;

              state = STATE_NONE;
              cur_token = NULL;

              continue;
            }

          ++cur_len;
          ++col;
          continue;
        }

      if (asprintf (&tmperror, "unknown lex state '%i'", state) < 0)
        tmperror = NULL;

      goto fail;
    }

  if (cur_token != NULL)
    {
      if (cur_token->kind == NITRO_TOKEN_IDENT)
        {
          if (nitro_ident_token_finalize (cur_token, str, cur_len,
                                          &token_list->string_arena,
                                          &tmperror))
            goto fail;
          goto done;
        }

      if (asprintf (&tmperror, "unknown token kind '%u'", cur_token->kind) < 0)
        tmperror = NULL;

      goto fail;
    }

done:
  TOKEN_ALLOC (cur_token);
  cur_token->kind = NITRO_TOKEN_EOF;

  return 0;

fail:
  nitro_token_list_free (token_list);

  if (error != NULL)
    {
      _opts->error.line = line;
      _opts->error.col = col;

      if (tmperror != NULL)
        *error = tmperror;
      else
        *error = NULL;
    }
  else if (tmperror)
    free (tmperror);

  return -1;
}

static const char *token_names[NITRO_TOKEN_COUNT] = {
#define STRINGIFY(X) [X] = #X,
  NITRO_TOKEN_KINDS (STRINGIFY)
};

static const char *kw_names[NITRO_KW_COUNT] = { NITRO_KW_KINDS (STRINGIFY)
#undef STRINGIFY
};

void
nitro_token_list_print (nitro_token_list *token_list)
{
  for (usize i = 0; i < token_list->len; ++i)
    {
      nitro_token *token = token_list->tokens + i;
      u16 kind = token->kind;

      if (kind <= NITRO_TOKEN_RESERVED || kind >= NITRO_TOKEN_COUNT)
        {
          printf ("<unknown token>\n");
          continue;
        }

      printf ("%s", token_names[kind]);

      if (token->kind == NITRO_TOKEN_KEYWORD)
        {
          u16 kw = token->kw;
          if (kw > NITRO_KW_RESERVED && kw < NITRO_KW_COUNT)
            printf (" %s", kw_names[kw]);
          else
            printf (" <unknown keyword>");
        }

      if (token->flags & NITRO_TOKEN_FLAG_STR)
        printf (" \"%.*s\"", (int) token->s.len, token->s.d);
      else if (token->flags & NITRO_TOKEN_FLAG_SSO)
        printf (" \"%.*s\"", (int) token->sso.len, token->sso.d);

      printf ("\n");
    }
}

void
nitro_token_list_free (nitro_token_list *token_list)
{
  nitro_arena_release (&token_list->token_arena);
  nitro_arena_release (&token_list->string_arena);
  token_list->len = 0;
  token_list->tokens = NULL;
}