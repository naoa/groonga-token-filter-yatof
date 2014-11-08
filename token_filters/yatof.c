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
  grn_obj *data;
  grn_tokenizer_status status;
  int max_length_in_bytes = 64;
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
}

static void
min_length_filter(grn_ctx *ctx,
                  grn_token *current_token,
                  grn_token *next_token,
                  GNUC_UNUSED void *user_data)
{
  grn_obj *data;
  grn_tokenizer_status status;
  int min_length_in_bytes = 3;
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
  return rc;
}

grn_rc
GRN_PLUGIN_FIN(GNUC_UNUSED grn_ctx *ctx)
{

  return GRN_SUCCESS;
}
