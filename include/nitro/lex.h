#ifndef NITROC_LEX_H
#define NITROC_LEX_H 1

#include "nitro/arena.h"
#include "nitro/types.h"

#define NITRO_TOKEN_KINDS(OP)                                                 \
  OP (NITRO_TOKEN_IDENT)                                                      \
  OP (NITRO_TOKEN_KEYWORD)                                                    \
  OP (NITRO_TOKEN_STRING)                                                     \
  OP (NITRO_TOKEN_INT)                                                        \
  OP (NITRO_TOKEN_FLOAT)                                                      \
  OP (NITRO_TOKEN_LPAREN)                                                     \
  OP (NITRO_TOKEN_RPAREN)                                                     \
  OP (NITRO_TOKEN_LBRACK)                                                     \
  OP (NITRO_TOKEN_RBRACK)                                                     \
  OP (NITRO_TOKEN_LBRACE)                                                     \
  OP (NITRO_TOKEN_RBRACE)                                                     \
  OP (NITRO_TOKEN_COMMA)                                                      \
  OP (NITRO_TOKEN_PERIOD)                                                     \
  OP (NITRO_TOKEN_EXCLAIM)                                                    \
  OP (NITRO_TOKEN_SEMICOLON)                                                  \
  OP (NITRO_TOKEN_ARROW)                                                      \
  OP (NITRO_TOKEN_PLUS)                                                       \
  OP (NITRO_TOKEN_PLUS_EQ)                                                    \
  OP (NITRO_TOKEN_MINUS)                                                      \
  OP (NITRO_TOKEN_MINUS_EQ)                                                   \
  OP (NITRO_TOKEN_STAR)                                                       \
  OP (NITRO_TOKEN_STAR_EQ)                                                    \
  OP (NITRO_TOKEN_DIVIDE)                                                     \
  OP (NITRO_TOKEN_DIVIDE_EQ)                                                  \
  OP (NITRO_TOKEN_MOD)                                                        \
  OP (NITRO_TOKEN_MOD_EQ)                                                     \
  OP (NITRO_TOKEN_EQ)                                                         \
  OP (NITRO_TOKEN_EQ_EQ)                                                      \
  OP (NITRO_TOKEN_NOT_EQ)                                                     \
  OP (NITRO_TOKEN_GR)                                                         \
  OP (NITRO_TOKEN_GR_EQ)                                                      \
  OP (NITRO_TOKEN_LS)                                                         \
  OP (NITRO_TOKEN_LS_EQ)                                                      \
  OP (NITRO_TOKEN_BIT_COMPL)                                                  \
  OP (NITRO_TOKEN_BIT_AND)                                                    \
  OP (NITRO_TOKEN_BIT_AND_EQ)                                                 \
  OP (NITRO_TOKEN_BIT_OR)                                                     \
  OP (NITRO_TOKEN_BIT_OR_EQ)                                                  \
  OP (NITRO_TOKEN_BIT_XOR)                                                    \
  OP (NITRO_TOKEN_BIT_XOR_EQ)                                                 \
  OP (NITRO_TOKEN_AND)                                                        \
  OP (NITRO_TOKEN_OR)                                                         \
  OP (NITRO_TOKEN_BUILTIN)                                                    \
  OP (NITRO_TOKEN_EOF)

#define NITRO_KW_KINDS(OP)                                                    \
  OP (NITRO_KW_BOOL)                                                          \
  OP (NITRO_KW_FALSE)                                                         \
  OP (NITRO_KW_TRUE)                                                          \
  OP (NITRO_KW_VOID)                                                          \
  OP (NITRO_KW_NULL)                                                          \
  OP (NITRO_KW_U8)                                                            \
  OP (NITRO_KW_U16)                                                           \
  OP (NITRO_KW_U32)                                                           \
  OP (NITRO_KW_U64)                                                           \
  OP (NITRO_KW_I8)                                                            \
  OP (NITRO_KW_I16)                                                           \
  OP (NITRO_KW_I32)                                                           \
  OP (NITRO_KW_I64)                                                           \
  OP (NITRO_KW_F32)                                                           \
  OP (NITRO_KW_F64)                                                           \
  OP (NITRO_KW_IF)                                                            \
  OP (NITRO_KW_FOR)                                                           \
  OP (NITRO_KW_WHILE)                                                         \
  OP (NITRO_KW_CONT)                                                          \
  OP (NITRO_KW_BRK)                                                           \
  OP (NITRO_KW_GOTO)                                                          \
  OP (NITRO_KW_ENUM)                                                          \
  OP (NITRO_KW_STRUCT)                                                        \
  OP (NITRO_KW_UNION)                                                         \
  OP (NITRO_KW_FN)                                                            \
  OP (NITRO_KW_PACKED)                                                        \
  OP (NITRO_KW_PUB)                                                           \
  OP (NITRO_KW_DEFER)

typedef enum
{
  NITRO_TOKEN_RESERVED = 0,
#define PASSTHROUGH(X) X,
  NITRO_TOKEN_KINDS (PASSTHROUGH) NITRO_TOKEN_COUNT,
} nitro_token_kind;

typedef enum
{
  NITRO_KW_RESERVED = 0,
  NITRO_KW_KINDS (PASSTHROUGH) NITRO_KW_COUNT,
#undef PASSTHROUGH
} nitro_keyword_kind;

extern const char *nitro_keyword_table[NITRO_KW_COUNT];

typedef struct
{
#define NITRO_TOKEN_FLAG_STR (1 << 0)
#define NITRO_TOKEN_FLAG_SSO (1 << 1)
  u16 kind, flags;
  u32 line, col;
  u16 kw;

  union
  {
    u64 i;
    f64 f;

    struct nitro_token_string
    {
      usize len;
      char *d;
    } s;

    struct
    {
      u8 len;
#define NITRO_TOKEN_SSO_MAX_LEN (sizeof (struct nitro_token_string) - 1)
      _Static_assert (NITRO_TOKEN_SSO_MAX_LEN >= 2,
                      "Token small string must hold at least two characters.");
      char d[NITRO_TOKEN_SSO_MAX_LEN];
    } sso;
  };
} nitro_token;

typedef struct
{
  usize len;
  nitro_token *tokens;
  nitro_arena token_arena, string_arena;
} nitro_token_list;

typedef struct
{
  u32 line, col;
  char *msg;
} nitro_lex_error;

typedef struct
{
  u64 token_arena_size;
  u64 strings_arena_size;
  nitro_lex_error error;
} nitro_lex_opts;

int nitro_lex (usize len, const char *str, nitro_token_list *token_list,
               nitro_lex_opts *opts);

void nitro_token_list_print (nitro_token_list *token_list);

void nitro_token_list_free (nitro_token_list *token_list);

#endif