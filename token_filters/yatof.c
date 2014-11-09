/* Copyright(C) 2014 Naoya Murakami <naoya@createfield.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301  USA
*/

#include <groonga/token_filter.h>
#include <groonga/nfkc.h>

#include <string.h>
#include <stdlib.h>

#ifdef __GNUC__
#  define GNUC_UNUSED __attribute__((__unused__))
#else
#  define GNUC_UNUSED
#endif

typedef struct {
  grn_obj *table;
  grn_token_mode mode;
  grn_tokenizer_token token;
} grn_yatof_token_filter;

static void *
yatof_init(grn_ctx *ctx, grn_obj *table, grn_token_mode mode)
{
  grn_yatof_token_filter *token_filter;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_yatof_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][yatof] "
                     "failed to allocate grn_yatof_token_filter");
    return NULL;
  }
  token_filter->table = table;
  token_filter->mode = mode;
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
yatof_fin(grn_ctx *ctx, void *user_data)
{
  grn_yatof_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

static void
max_length_filter(grn_ctx *ctx,
                  grn_token *current_token,
                  grn_token *next_token,
                  GNUC_UNUSED void *user_data)
{
#define DEFAULT_MAX_LENGTH 64
  grn_obj *data;
  grn_tokenizer_status status;
  int max_length_in_bytes = DEFAULT_MAX_LENGTH;
  const char *max_length_env;
  data = grn_token_get_data(ctx, current_token);

  max_length_env = getenv("GRN_YATOF_MAX_TOKEN_LENGTH");
  if (max_length_env) {
    max_length_in_bytes = atoi(max_length_env);
  }
  if (GRN_TEXT_LEN(data) > max_length_in_bytes) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKENIZER_TOKEN_SKIP;
    grn_token_set_status(ctx, next_token, status);
  }
#undef DEFAULT_MAX_LENGTH
}

static void
min_length_filter(grn_ctx *ctx,
                  grn_token *current_token,
                  grn_token *next_token,
                  GNUC_UNUSED void *user_data)
{
  grn_obj *data;
#define DEFAULT_MIN_LENGTH 3
  grn_tokenizer_status status;
  int min_length_in_bytes = DEFAULT_MIN_LENGTH;
  const char *min_length_env;
  data = grn_token_get_data(ctx, current_token);

  min_length_env = getenv("GRN_YATOF_MIN_TOKEN_LENGTH");
  if (min_length_env) {
    min_length_in_bytes = atoi(min_length_env);
  }
  if (GRN_TEXT_LEN(data) < min_length_in_bytes) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKENIZER_TOKEN_SKIP;
    grn_token_set_status(ctx, next_token, status);
  }
#undef DEFAULT_MIN_LENGTH
}

static void
prolong_filter(grn_ctx *ctx,
               grn_token *current_token,
               grn_token *next_token,
               GNUC_UNUSED void *user_data)
{
#define CUT_PROLONG_LENGTH 4

  grn_obj *data;
  int token_size = 0;
  grn_bool is_katakana = GRN_TRUE;

  data = grn_token_get_data(ctx, current_token);

  {
    int char_length;
    int rest_length = GRN_TEXT_LEN(data);
    const char *rest = GRN_TEXT_VALUE(data);

    while (rest_length > 0) {
      grn_char_type type;
      grn_encoding encoding = GRN_CTX_GET_ENCODING(ctx);
      char_length = grn_plugin_charlen(ctx, rest, rest_length, encoding);
      if (char_length == 0) {
        break;
      }
      type = grn_nfkc_char_type((unsigned char *)rest);
      if (type != GRN_CHAR_KATAKANA) {
        is_katakana = GRN_FALSE;
        break;
      }
      token_size++;
      rest += char_length;
      rest_length -= char_length;
    }
  }
  if (is_katakana && token_size >= CUT_PROLONG_LENGTH) {
    const char *last = GRN_TEXT_VALUE(data);
    last += GRN_TEXT_LEN(data) - 3;
    if (!memcmp("ー", last, 3)) {
      grn_token_set_data(ctx, next_token,
                         GRN_TEXT_VALUE(data),
                         GRN_TEXT_LEN(data) - 3);
    }
  }
#undef CUT_PROLONG_LENGTH
}

static void
symbol_filter(grn_ctx *ctx,
              grn_token *current_token,
              grn_token *next_token,
              GNUC_UNUSED void *user_data)
{
  grn_obj *data;
  grn_tokenizer_status status;
  int token_size = 0;
  grn_bool is_symbol = GRN_TRUE;

  data = grn_token_get_data(ctx, current_token);

  {
    int char_length;
    int rest_length = GRN_TEXT_LEN(data);
    const char *rest = GRN_TEXT_VALUE(data);

    while (rest_length > 0) {
      grn_char_type type;
      grn_encoding encoding = GRN_CTX_GET_ENCODING(ctx);
      char_length = grn_plugin_charlen(ctx, rest, rest_length, encoding);
      if (char_length == 0) {
        break;
      }
      type = grn_nfkc_char_type((unsigned char *)rest);
      if (type != GRN_CHAR_SYMBOL) {
        is_symbol = GRN_FALSE;
        break;
      }
      token_size++;
      rest += char_length;
      rest_length -= char_length;
    }
  }
  if (is_symbol) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKENIZER_TOKEN_SKIP;
    grn_token_set_status(ctx, next_token, status);
  }
}

static void
digit_filter(grn_ctx *ctx,
             grn_token *current_token,
             grn_token *next_token,
             GNUC_UNUSED void *user_data)
{
  grn_obj *data;
  grn_tokenizer_status status;
  int token_size = 0;
  grn_bool is_digit = GRN_TRUE;

  data = grn_token_get_data(ctx, current_token);

  {
    int char_length;
    int rest_length = GRN_TEXT_LEN(data);
    const char *rest = GRN_TEXT_VALUE(data);

    while (rest_length > 0) {
      grn_char_type type;
      grn_encoding encoding = GRN_CTX_GET_ENCODING(ctx);
      char_length = grn_plugin_charlen(ctx, rest, rest_length, encoding);
      if (char_length == 0) {
        break;
      }
      type = grn_nfkc_char_type((unsigned char *)rest);
      if (type != GRN_CHAR_DIGIT) {
        is_digit = GRN_FALSE;
        break;
      }
      token_size++;
      rest += char_length;
      rest_length -= char_length;
    }
  }
  if (is_digit) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKENIZER_TOKEN_SKIP;
    grn_token_set_status(ctx, next_token, status);
  }
}

typedef struct {
  grn_tokenizer_token token;
  grn_obj *table;
  grn_obj value;
} grn_tf_limit_token_filter;

static void *
tf_limit_init(grn_ctx *ctx, GNUC_UNUSED grn_obj *table, GNUC_UNUSED grn_token_mode mode)
{
  grn_tf_limit_token_filter *token_filter;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_tf_limit_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][tf-limit] "
                     "failed to allocate grn_tf_limit_token_filter");
    return NULL;
  }
  token_filter->table = grn_table_create(ctx, NULL, 0, NULL,
                                         GRN_OBJ_TABLE_HASH_KEY,
                                         grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                                         grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!token_filter->table) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][tf-limit] "
                     "couldn't create a table");
    GRN_PLUGIN_FREE(ctx, token_filter);
    return NULL;
  }

  GRN_UINT32_INIT(&(token_filter->value), 0);
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
tf_limit_filter(grn_ctx *ctx,
                grn_token *current_token,
                grn_token *next_token,
                void *user_data)
{
#define DEFAULT_TF_LIMIT 65535
  grn_tf_limit_token_filter *token_filter = user_data;
  grn_obj *data;
  grn_tokenizer_status status;
  unsigned int tf_limit = DEFAULT_TF_LIMIT;
  const char *tf_limit_env;
  data = grn_token_get_data(ctx, current_token);

  tf_limit_env = getenv("GRN_YATOF_TOKEN_LIMIT");
  if (tf_limit_env) {
    tf_limit = atoi(tf_limit_env);
  }

  {
    grn_id id;
    id = grn_table_add(ctx, token_filter->table,
                       GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data), NULL);
    if (id) {
      GRN_BULK_REWIND(&(token_filter->value));
      grn_obj_get_value(ctx, token_filter->table, id, &(token_filter->value));
      GRN_UINT32_SET(ctx, &(token_filter->value), GRN_UINT32_VALUE(&(token_filter->value)) + 1);
      grn_obj_set_value(ctx, token_filter->table, id, &(token_filter->value), GRN_OBJ_SET);
    }
  }
  if (GRN_UINT32_VALUE(&(token_filter->value)) > tf_limit) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKENIZER_TOKEN_SKIP;
    grn_token_set_status(ctx, next_token, status);
  }
#undef DEFAULT_TF_LIMIT
}

static void
tf_limit_fin(grn_ctx *ctx, void *user_data)
{
  grn_tf_limit_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  if (token_filter->table) {
    grn_obj_unlink(ctx, token_filter->table);
  }
  grn_obj_unlink(ctx, &(token_filter->value));
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

#define IGNORE_WORD_TABLE_NAME "#ignore_words"
#define IGNORE_WORD_TABLE_NAME_MRN "@0023ignore_words"

typedef struct {
  grn_tokenizer_token token;
  grn_obj *table;
  grn_obj value;
} grn_ignore_word_token_filter;

static void *
ignore_word_init(grn_ctx *ctx, GNUC_UNUSED grn_obj *table, GNUC_UNUSED grn_token_mode mode)
{
  grn_ignore_word_token_filter *token_filter;
  const char *ignore_word_table_name_env;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_ignore_word_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][ignore-word] "
                     "failed to allocate grn_ignore_word_token_filter");
    return NULL;
  }
  ignore_word_table_name_env = getenv("GRN_YATOF_IGNORE_WORD_TABLE_NAME");
  if (ignore_word_table_name_env) {
    token_filter->table = grn_ctx_get(ctx,
                                      ignore_word_table_name_env,
                                      strlen(ignore_word_table_name_env));
  } else {
    token_filter->table = grn_ctx_get(ctx,
                                      IGNORE_WORD_TABLE_NAME,
                                      strlen(IGNORE_WORD_TABLE_NAME));
    if (!token_filter->table) {
      token_filter->table = grn_ctx_get(ctx,
                                        IGNORE_WORD_TABLE_NAME_MRN,
                                        strlen(IGNORE_WORD_TABLE_NAME_MRN));
    }
  }
  if (!token_filter->table) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][ignore-word] "
                     "couldn't open a table");
    GRN_PLUGIN_FREE(ctx, token_filter);
    return NULL;
  }

  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
ignore_word_filter(grn_ctx *ctx,
                   grn_token *current_token,
                   grn_token *next_token,
                   void *user_data)
{
  grn_ignore_word_token_filter *token_filter = user_data;
  grn_obj *data;
  grn_tokenizer_status status;
  data = grn_token_get_data(ctx, current_token);

  {
    grn_id id;
    id = grn_table_get(ctx, token_filter->table,
                       GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));
    if (id != GRN_ID_NIL) {
      status = grn_token_get_status(ctx, current_token);
      status |= GRN_TOKENIZER_TOKEN_SKIP_WITH_POSITION;
      grn_token_set_status(ctx, next_token, status);
    }
  }
}

static void
ignore_word_fin(grn_ctx *ctx, void *user_data)
{
  grn_ignore_word_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  if (token_filter->table) {
    grn_obj_unlink(ctx, token_filter->table);
  }
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc;

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterMaxLength", -1,
                                 yatof_init,
                                 max_length_filter,
                                 yatof_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterMinLength", -1,
                                 yatof_init,
                                 min_length_filter,
                                 yatof_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterTFLimit", -1,
                                 tf_limit_init,
                                 tf_limit_filter,
                                 tf_limit_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterProlong", -1,
                                 yatof_init,
                                 prolong_filter,
                                 yatof_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterSymbol", -1,
                                 yatof_init,
                                 symbol_filter,
                                 yatof_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterDigit", -1,
                                 yatof_init,
                                 digit_filter,
                                 yatof_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterIgnoreWord", -1,
                                 ignore_word_init,
                                 ignore_word_filter,
                                 ignore_word_fin);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(GNUC_UNUSED grn_ctx *ctx)
{

  return GRN_SUCCESS;
}
