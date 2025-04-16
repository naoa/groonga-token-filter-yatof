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

typedef struct {
  grn_obj *table;
  grn_token_mode mode;
  grn_tokenizer_token token;
  int max_length_in_bytes;
} grn_max_length_token_filter;

static void *
max_length_init(grn_ctx *ctx, grn_obj *table, grn_token_mode mode)
{
#define DEFAULT_MAX_LENGTH 64
  grn_max_length_token_filter *token_filter;
  const char *max_length_env;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_max_length_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][max-length] "
                     "failed to allocate grn_max_length_token_filter");
    return NULL;
  }
  max_length_env = getenv("GRN_YATOF_MAX_TOKEN_LENGTH");
  if (max_length_env) {
    token_filter->max_length_in_bytes = atoi(max_length_env);
  } else {
    token_filter->max_length_in_bytes = DEFAULT_MAX_LENGTH;
  }
  token_filter->table = table;
  token_filter->mode = mode;
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
#undef DEFAULT_MAX_LENGTH
}

static void
max_length_filter(grn_ctx *ctx,
                  grn_token *current_token,
                  grn_token *next_token,
                  void *user_data)
{
  grn_obj *data;
  grn_max_length_token_filter *token_filter = user_data;
  grn_tokenizer_status status;
  data = grn_token_get_data(ctx, current_token);

  if (GRN_TEXT_LEN(data) > token_filter->max_length_in_bytes) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKEN_SKIP_WITH_POSITION;
    grn_token_set_status(ctx, next_token, status);
  }
}

static void
max_length_fin(grn_ctx *ctx, void *user_data)
{
  grn_max_length_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

typedef struct {
  grn_obj *table;
  grn_token_mode mode;
  grn_tokenizer_token token;
  int min_length_in_bytes;
} grn_min_length_token_filter;

static void *
min_length_init(grn_ctx *ctx, grn_obj *table, grn_token_mode mode)
{
#define DEFAULT_MIN_LENGTH 3
  grn_min_length_token_filter *token_filter;
  const char *min_length_env;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_min_length_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][min-length] "
                     "failed to allocate grn_min_length_token_filter");
    return NULL;
  }
  min_length_env = getenv("GRN_YATOF_MIN_TOKEN_LENGTH");
  if (min_length_env) {
    token_filter->min_length_in_bytes = atoi(min_length_env);
  } else {
    token_filter->min_length_in_bytes = DEFAULT_MIN_LENGTH;
  }
  token_filter->table = table;
  token_filter->mode = mode;
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
#undef DEFAULT_MIN_LENGTH
}

static void
min_length_filter(grn_ctx *ctx,
                  grn_token *current_token,
                  grn_token *next_token,
                  void *user_data)
{
  grn_obj *data;
  grn_min_length_token_filter *token_filter = user_data;
  grn_tokenizer_status status;
  data = grn_token_get_data(ctx, current_token);

  if (GRN_TEXT_LEN(data) < token_filter->min_length_in_bytes) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKEN_SKIP_WITH_POSITION;
    grn_token_set_status(ctx, next_token, status);
  }
}

static void
min_length_fin(grn_ctx *ctx, void *user_data)
{
  grn_min_length_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
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
      if (type != GRN_CHAR_SYMBOL && type != GRN_CHAR_OTHERS) {
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
    status |= GRN_TOKEN_SKIP_WITH_POSITION;
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
    status |= GRN_TOKEN_SKIP_WITH_POSITION;
    grn_token_set_status(ctx, next_token, status);
  }
}

static void
unmatured_one_filter(grn_ctx *ctx,
             grn_token *current_token,
             grn_token *next_token,
             GNUC_UNUSED void *user_data)
{
  grn_obj *data;
  grn_tokenizer_status status;
  int token_size = 0;

  data = grn_token_get_data(ctx, current_token);

  {
    int char_length;
    int rest_length = GRN_TEXT_LEN(data);
    const char *rest = GRN_TEXT_VALUE(data);

    while (rest_length > 0) {
      grn_encoding encoding = GRN_CTX_GET_ENCODING(ctx);
      char_length = grn_plugin_charlen(ctx, rest, rest_length, encoding);
      if (char_length == 0) {
        break;
      }
      token_size++;
      rest += char_length;
      rest_length -= char_length;
    }
  }
  if (token_size == 1) {
    status = grn_token_get_status(ctx, current_token);
    if (status & GRN_TOKEN_UNMATURED) {
      status |= GRN_TOKEN_SKIP_WITH_POSITION;
      grn_token_set_status(ctx, next_token, status);
    }
  }
}

#define ATGC_LIMIT 9

static void
atgc_filter(grn_ctx *ctx,
             grn_token *current_token,
             grn_token *next_token,
             GNUC_UNUSED void *user_data)
{
  grn_obj *data;
  grn_tokenizer_status status;
  int token_size = 0;
  int n_atgc = 0;

  data = grn_token_get_data(ctx, current_token);

  {
    int char_length;
    int rest_length = GRN_TEXT_LEN(data);
    const char *rest = GRN_TEXT_VALUE(data);

    while (rest_length > 0) {
      grn_encoding encoding = GRN_CTX_GET_ENCODING(ctx);
      char_length = grn_plugin_charlen(ctx, rest, rest_length, encoding);
      if (char_length == 0) {
        break;
      }
      if (rest[0] == 'A' || rest[0] == 'T' || rest[0] == 'G' || rest[0] == 'C' || rest[0] == 'U' ||
          rest[0] == 'a' || rest[0] == 't' || rest[0] == 'g' || rest[0] == 'c' || rest[0] == 'u') {
        n_atgc++;
      } else {
        n_atgc = 0;
      }
      token_size++;
      rest += char_length;
      rest_length -= char_length;
    }
  }
  if (n_atgc >= ATGC_LIMIT) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKEN_SKIP_WITH_POSITION;
    grn_token_set_status(ctx, next_token, status);
  }
}

#define TF_LIMIT_WORD_TABLE_NAME "tf_limits"
#define TF_LIMIT_COLUMN_NAME "tf_limit"

typedef struct {
  grn_tokenizer_token token;
  grn_obj *table;
  grn_obj value;
  unsigned int tf_limit;
  grn_obj *word_table;
  grn_obj *column;
  grn_obj word_tf_limit;
} grn_tf_limit_token_filter;

static void *
tf_limit_init(grn_ctx *ctx, GNUC_UNUSED grn_obj *table, GNUC_UNUSED grn_token_mode mode)
{
#define DEFAULT_TF_LIMIT 131071
  grn_tf_limit_token_filter *token_filter;
  const char *tf_limit_env;
  const char *tf_limit_word_table_name_env;

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
  tf_limit_env = getenv("GRN_YATOF_TF_LIMIT");
  if (tf_limit_env) {
    token_filter->tf_limit = atoi(tf_limit_env);
  } else {
    token_filter->tf_limit = DEFAULT_TF_LIMIT;
  }
  token_filter->word_table = NULL;
  token_filter->column = NULL;

  tf_limit_word_table_name_env = getenv("GRN_YATOF_TF_LIMIT_WORD_TABLE_NAME");
  if (tf_limit_word_table_name_env) {
    token_filter->word_table = grn_ctx_get(ctx,
                                           tf_limit_word_table_name_env,
                                           strlen(tf_limit_word_table_name_env));
  } else {
    token_filter->word_table = grn_ctx_get(ctx,
                                           TF_LIMIT_WORD_TABLE_NAME,
                                           strlen(TF_LIMIT_WORD_TABLE_NAME));
  }

  if (token_filter->word_table) {
     token_filter->column = grn_obj_column(ctx,
                                           token_filter->word_table,
                                           TF_LIMIT_COLUMN_NAME,
                                           strlen(TF_LIMIT_COLUMN_NAME));
  }

  GRN_UINT32_INIT(&(token_filter->value), 0);
  GRN_UINT32_INIT(&(token_filter->word_tf_limit), 0);
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
#undef DEFAULT_TF_LIMIT
}

static void
tf_limit_filter(grn_ctx *ctx,
                grn_token *current_token,
                grn_token *next_token,
                void *user_data)
{
  grn_tf_limit_token_filter *token_filter = user_data;
  grn_obj *data;
  grn_tokenizer_status status;
  data = grn_token_get_data(ctx, current_token);
  unsigned int tf_limit = token_filter->tf_limit;

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

  if (token_filter->word_table) {
    grn_id id;
    id = grn_table_get(ctx, token_filter->word_table,
                       GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));
    if (id != GRN_ID_NIL) {
      GRN_BULK_REWIND(&(token_filter->word_tf_limit));
      grn_obj_get_value(ctx, token_filter->column, id, &(token_filter->word_tf_limit));
      tf_limit = GRN_UINT32_VALUE(&(token_filter->word_tf_limit));
    }
  }

  if (GRN_UINT32_VALUE(&(token_filter->value)) > tf_limit) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKEN_SKIP_WITH_POSITION;
    grn_token_set_status(ctx, next_token, status);

    GRN_PLUGIN_LOG(ctx, GRN_LOG_INFO,
                   "[token-filter][tf-limit] "
                   "<%.*s> is reached on tf limit %u",
                   (int)GRN_TEXT_LEN(data), GRN_TEXT_VALUE(data),
                   tf_limit);
  }
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
  if (token_filter->word_table) {
    grn_obj_unlink(ctx, token_filter->word_table);
  }
  if (token_filter->column) {
    grn_obj_unlink(ctx, token_filter->column);
  }
  grn_obj_unlink(ctx, &(token_filter->value));
  grn_obj_unlink(ctx, &(token_filter->word_tf_limit));
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

typedef struct {
  grn_tokenizer_token token;
  grn_obj *table;
  grn_obj value;
  grn_obj previous_token;
  unsigned int phrase_limit;
} grn_phrase_limit_token_filter;

static void *
phrase_limit_init(grn_ctx *ctx, GNUC_UNUSED grn_obj *table, GNUC_UNUSED grn_token_mode mode)
{
#define DEFAULT_PHRASE_LIMIT 4096
  grn_phrase_limit_token_filter *token_filter;
  const char *phrase_limit_env;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_phrase_limit_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][phrase-limit] "
                     "failed to allocate grn_phrase_limit_token_filter");
    return NULL;
  }
  token_filter->table = grn_table_create(ctx, NULL, 0, NULL,
                                         GRN_OBJ_TABLE_HASH_KEY,
                                         grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                                         grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!token_filter->table) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][phrase-limit] "
                     "couldn't create a table");
    GRN_PLUGIN_FREE(ctx, token_filter);
    return NULL;
  }
  phrase_limit_env = getenv("GRN_YATOF_PHRASE_LIMIT");
  if (phrase_limit_env) {
    token_filter->phrase_limit = atoi(phrase_limit_env);
  } else {
    token_filter->phrase_limit = DEFAULT_PHRASE_LIMIT;
  }

  GRN_UINT32_INIT(&(token_filter->value), 0);
  GRN_TEXT_INIT(&(token_filter->previous_token), 0);
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
#undef DEFAULT_PHRASE_LIMIT
}

static void
phrase_limit_filter(grn_ctx *ctx,
                grn_token *current_token,
                grn_token *next_token,
                void *user_data)
{
  grn_phrase_limit_token_filter *token_filter = user_data;
  grn_obj *data;
  grn_tokenizer_status status;
  data = grn_token_get_data(ctx, current_token);
  unsigned int phrase_limit = token_filter->phrase_limit;

  if (GRN_TEXT_LEN(&(token_filter->previous_token)) != 0) {
    grn_id id;
    GRN_TEXT_PUT(ctx, &(token_filter->previous_token),
                 GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));

    id = grn_table_add(ctx, token_filter->table,
                       GRN_TEXT_VALUE(&(token_filter->previous_token)),
                       GRN_TEXT_LEN(&(token_filter->previous_token)), NULL);
    if (id) {
      GRN_BULK_REWIND(&(token_filter->value));
      grn_obj_get_value(ctx, token_filter->table, id, &(token_filter->value));
      GRN_UINT32_SET(ctx, &(token_filter->value), GRN_UINT32_VALUE(&(token_filter->value)) + 1);
      grn_obj_set_value(ctx, token_filter->table, id, &(token_filter->value), GRN_OBJ_SET);
    }
  }
  GRN_BULK_REWIND(&(token_filter->previous_token));
  GRN_TEXT_SET(ctx, &(token_filter->previous_token),
               GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));

  if (GRN_UINT32_VALUE(&(token_filter->value)) > phrase_limit) {
    status = grn_token_get_status(ctx, current_token);
    status |= GRN_TOKEN_SKIP_WITH_POSITION;
    grn_token_set_status(ctx, next_token, status);

    GRN_PLUGIN_LOG(ctx, GRN_LOG_NOTICE,
                   "[token-filter][phrase-limit] "
                   "<%.*s> is reached on phrase limit %u",
                   (int)GRN_TEXT_LEN(data), GRN_TEXT_VALUE(data),
                   phrase_limit);
  }
}

static void
phrase_limit_fin(grn_ctx *ctx, void *user_data)
{
  grn_phrase_limit_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  if (token_filter->table) {
    grn_obj_unlink(ctx, token_filter->table);
  }
  grn_obj_unlink(ctx, &(token_filter->value));
  grn_obj_unlink(ctx, &(token_filter->previous_token));
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

#define IGNORE_WORD_TABLE_NAME "ignore_words"

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
      status |= GRN_TOKEN_SKIP_WITH_POSITION;
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

#define REMOVE_WORD_TABLE_NAME "remove_words"

typedef struct {
  grn_tokenizer_token token;
  grn_obj *table;
  grn_obj value;
} grn_remove_word_token_filter;

static void *
remove_word_init(grn_ctx *ctx, GNUC_UNUSED grn_obj *table, GNUC_UNUSED grn_token_mode mode)
{
  grn_remove_word_token_filter *token_filter;
  const char *remove_word_table_name_env;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_remove_word_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][remove-word] "
                     "failed to allocate grn_remove_word_token_filter");
    return NULL;
  }
  remove_word_table_name_env = getenv("GRN_YATOF_REMOVE_WORD_TABLE_NAME");
  if (remove_word_table_name_env) {
    token_filter->table = grn_ctx_get(ctx,
                                      remove_word_table_name_env,
                                      strlen(remove_word_table_name_env));
  } else {
    token_filter->table = grn_ctx_get(ctx,
                                      REMOVE_WORD_TABLE_NAME,
                                      strlen(REMOVE_WORD_TABLE_NAME));
  }
  if (!token_filter->table) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][remove-word] "
                     "couldn't open a table");
    GRN_PLUGIN_FREE(ctx, token_filter);
    return NULL;
  }

  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
remove_word_filter(grn_ctx *ctx,
                   grn_token *current_token,
                   grn_token *next_token,
                   void *user_data)
{
  grn_remove_word_token_filter *token_filter = user_data;
  grn_obj *data;
  grn_tokenizer_status status;
  data = grn_token_get_data(ctx, current_token);

  {
    grn_id id;
    id = grn_table_get(ctx, token_filter->table,
                       GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));
    if (id != GRN_ID_NIL) {
      status = grn_token_get_status(ctx, current_token);
      status |= GRN_TOKEN_SKIP;
      grn_token_set_status(ctx, next_token, status);
    }
  }
}

static void
remove_word_fin(grn_ctx *ctx, void *user_data)
{
  grn_remove_word_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  if (token_filter->table) {
    grn_obj_unlink(ctx, token_filter->table);
  }
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

#define THROUGH_WORD_TABLE_NAME "through_words"

typedef struct {
  grn_tokenizer_token token;
  grn_obj *table;
  grn_obj value;
} grn_through_word_token_filter;

static void *
through_word_init(grn_ctx *ctx, GNUC_UNUSED grn_obj *table, GNUC_UNUSED grn_token_mode mode)
{
  grn_through_word_token_filter *token_filter;
  const char *through_word_table_name_env;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_through_word_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][through-word] "
                     "failed to allocate grn_through_word_token_filter");
    return NULL;
  }
  through_word_table_name_env = getenv("GRN_YATOF_THROUGH_WORD_TABLE_NAME");
  if (through_word_table_name_env) {
    token_filter->table = grn_ctx_get(ctx,
                                      through_word_table_name_env,
                                      strlen(through_word_table_name_env));
  } else {
    token_filter->table = grn_ctx_get(ctx,
                                      THROUGH_WORD_TABLE_NAME,
                                      strlen(THROUGH_WORD_TABLE_NAME));
  }
  if (!token_filter->table) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][through-word] "
                     "couldn't open a table");
    GRN_PLUGIN_FREE(ctx, token_filter);
    return NULL;
  }

  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
through_word_filter(grn_ctx *ctx,
                   grn_token *current_token,
                   grn_token *next_token,
                   void *user_data)
{
  grn_through_word_token_filter *token_filter = user_data;
  grn_obj *data;
  grn_tokenizer_status status;
  data = grn_token_get_data(ctx, current_token);

  {
    grn_id id;
    id = grn_table_get(ctx, token_filter->table,
                       GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));
    if (id == GRN_ID_NIL) {
      status = grn_token_get_status(ctx, current_token);
      status |= GRN_TOKEN_SKIP_WITH_POSITION;
      grn_token_set_status(ctx, next_token, status);
    }
  }
}

static void
through_word_fin(grn_ctx *ctx, void *user_data)
{
  grn_through_word_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  if (token_filter->table) {
    grn_obj_unlink(ctx, token_filter->table);
  }
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}

#define SYNONYM_TABLE_NAME "synonyms"
#define SYNONYM_COLUMN_NAME "synonym"

typedef struct {
  grn_tokenizer_token token;
  grn_obj *table;
  grn_obj *column;
  grn_obj value;
} grn_synonym_token_filter;

static void *
synonym_init(grn_ctx *ctx, GNUC_UNUSED grn_obj *table, GNUC_UNUSED grn_token_mode mode)
{
  grn_synonym_token_filter *token_filter;
  const char *synonym_table_name_env;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_synonym_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][synonym] "
                     "failed to allocate grn_synonym_token_filter");
    return NULL;
  }
  synonym_table_name_env = getenv("GRN_YATOF_SYNONYM_TABLE_NAME");
  if (synonym_table_name_env) {
    token_filter->table = grn_ctx_get(ctx,
                                      synonym_table_name_env,
                                      strlen(synonym_table_name_env));
  } else {
    token_filter->table = grn_ctx_get(ctx,
                                      SYNONYM_TABLE_NAME,
                                      strlen(SYNONYM_TABLE_NAME));
  }
  if (!token_filter->table) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][synonym] "
                     "couldn't open a table");
    GRN_PLUGIN_FREE(ctx, token_filter);
    return NULL;
  } else {
    token_filter->column = grn_obj_column(ctx,
                                          token_filter->table,
                                          SYNONYM_COLUMN_NAME,
                                          strlen(SYNONYM_COLUMN_NAME));
    if (!token_filter->column) {
      GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                       "[token-filter][synonym] "
                       "couldn't open synonym column");
      GRN_PLUGIN_FREE(ctx, token_filter);
      return NULL;
    }
  }

  GRN_TEXT_INIT(&(token_filter->value), 0);
  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
synonym_filter(grn_ctx *ctx,
               grn_token *current_token,
               grn_token *next_token,
               void *user_data)
{
  grn_synonym_token_filter *token_filter = user_data;
  grn_obj *data;
  data = grn_token_get_data(ctx, current_token);

  {
    grn_id id;
    id = grn_table_get(ctx, token_filter->table,
                       GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));
    if (id != GRN_ID_NIL) {
      GRN_BULK_REWIND(&(token_filter->value));
      grn_obj_get_value(ctx, token_filter->column, id, &(token_filter->value));
      grn_token_set_data(ctx, next_token,
                         GRN_TEXT_VALUE(&(token_filter->value)),
                         GRN_TEXT_LEN(&(token_filter->value)));
    }
  }
}

static void
synonym_fin(grn_ctx *ctx, void *user_data)
{
  grn_synonym_token_filter *token_filter = user_data;
  if (!token_filter) {
    return;
  }
  if (token_filter->table) {
    grn_obj_unlink(ctx, token_filter->table);
  }
  if (token_filter->column) {
    grn_obj_unlink(ctx, token_filter->column);
  }
  grn_obj_unlink(ctx, &(token_filter->value));
  grn_tokenizer_token_fin(ctx, &(token_filter->token));
  GRN_PLUGIN_FREE(ctx, token_filter);
}


const char *white_table_name = "white_terms";
uint32_t white_table_name_size = 11;

typedef struct {
  grn_tokenizer_token token;
  grn_obj *table;
  grn_token_mode mode;
} grn_white_token_filter;

static void *
white_init(grn_ctx *ctx, GNUC_UNUSED grn_obj *table, GNUC_UNUSED grn_token_mode mode)
{
  grn_white_token_filter *token_filter;

  token_filter = GRN_PLUGIN_MALLOC(ctx, sizeof(grn_white_token_filter));
  if (!token_filter) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][white] "
                     "failed to allocate grn_white_token_filter");
    return NULL;
  }
  token_filter->table = grn_ctx_get(ctx,
                                    white_table_name,
                                    white_table_name_size);
  if (!token_filter->table) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token-filter][white] "
                     "couldn't open a table");
    GRN_PLUGIN_FREE(ctx, token_filter);
    return NULL;
  }
  token_filter->mode = mode;

  grn_tokenizer_token_init(ctx, &(token_filter->token));

  return token_filter;
}

static void
white_filter(grn_ctx *ctx,
             grn_token *current_token,
             grn_token *next_token,
             void *user_data)
{
  grn_white_token_filter *token_filter = user_data;
  grn_obj *data;
  data = grn_token_get_data(ctx, current_token);

  {
    grn_id id;
    id = grn_table_get(ctx, token_filter->table,
                       GRN_TEXT_VALUE(data), GRN_TEXT_LEN(data));
    if (id == GRN_ID_NIL) {
      grn_tokenizer_status status;
      status = grn_token_get_status(ctx, current_token);
      status |= GRN_TOKEN_SKIP;
      grn_token_set_status(ctx, next_token, status);
    }
  }
}

static void
white_fin(grn_ctx *ctx, void *user_data)
{
  grn_white_token_filter *token_filter = user_data;
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
  {
    const char *config_table_name;
    uint32_t config_table_name_size;
    grn_config_get(ctx,
                   "tokenfilter-white.table", -1,
                   &config_table_name, &config_table_name_size);
    if (config_table_name) {
      white_table_name = config_table_name;
      white_table_name_size = config_table_name_size;
    }
  }
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc;

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterMaxLength", -1,
                                 max_length_init,
                                 max_length_filter,
                                 max_length_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterMinLength", -1,
                                 min_length_init,
                                 min_length_filter,
                                 min_length_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterTFLimit", -1,
                                 tf_limit_init,
                                 tf_limit_filter,
                                 tf_limit_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterPhraseLimit", -1,
                                 phrase_limit_init,
                                 phrase_limit_filter,
                                 phrase_limit_fin);

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

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterRemoveWord", -1,
                                 remove_word_init,
                                 remove_word_filter,
                                 remove_word_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterThroughWord", -1,
                                 through_word_init,
                                 through_word_filter,
                                 through_word_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterSynonym", -1,
                                 synonym_init,
                                 synonym_filter,
                                 synonym_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterUnmaturedOne", -1,
                                 yatof_init,
                                 unmatured_one_filter,
                                 yatof_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterWhite", -1,
                                 white_init,
                                 white_filter,
                                 white_fin);

  rc = grn_token_filter_register(ctx,
                                 "TokenFilterATGC", -1,
                                 yatof_init,
                                 atgc_filter,
                                 yatof_fin);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(GNUC_UNUSED grn_ctx *ctx)
{

  return GRN_SUCCESS;
}
