#include "json.h"
#include "array_size.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum offset
{
  PARSER_OFFSET = 0,
  CURSOR_OFFSET = 1,
  NODE_OFFSET = 2,
};

static inline struct json_parser *get_parser(struct json *x)
{
  return &x[PARSER_OFFSET].parser;
}

static inline struct json_cursor *cursor(struct json *x)
{
  return &x[CURSOR_OFFSET].cursor;
}

static inline struct json_node *nodes(struct json *x)
{
  return &x[NODE_OFFSET].node;
}

static inline struct json_node *cnode(struct json *x)
{
  return nodes(x) + cursor(x)->pos;
}

static inline struct json_node *sentinel(struct json *x)
{
  return &nodes(x)[get_parser(x)->size];
}

static inline void delimit(struct json *x)
{
  cursor(x)->json[cnode(x)->end] = '\0';
}

static inline void record_errno(struct json *x)
{
  if (errno == EINVAL) x->err = JSON_INVAL;
  if (errno == ERANGE) x->err = JSON_OUTRANGE;
}

static inline char *cstring(struct json *x)
{
  return &cursor(x)->json[cnode(x)->start];
}

static inline char *empty_string(struct json *x)
{
  return &cursor(x)->json[cursor(x)->length];
}

static void sentinel_init(struct json *);
static long strto_long(const char *restrict, char **restrict, int);
static unsigned long strto_ulong(const char *restrict, char **restrict, int);
static double strto_double(const char *restrict, char **restrict);
static int __strlcpy(char *dst, const char *src, int size);
static void parser_init(struct json_parser *, int size);
static void parser_reset(struct json_parser *);
static int parser_parse(struct json_parser *, int length, char *json,
                        int nnodes, struct json_node *);
static void cursor_init(struct json_cursor *cursor, int length, char *json);

void json_init(struct json *x, int alloc_size)
{
  x->err = 0;
  parser_init(get_parser(x), alloc_size);
}

int json_parse(struct json *x, int length, char *json)
{
  x->err = 0;
  parser_reset(get_parser(x));
  cursor_init(cursor(x), length, json);
  struct json_parser *p = get_parser(x);
  struct json_cursor *c = cursor(x);
  int n = p->alloc_size - 2;
  if ((x->err = parser_parse(p, c->length, c->json, n, nodes(x))))
    return x->err;
  sentinel_init(x);
  if (p->size > 0) cnode(x)->parent = -1;
  return x->err;
}

int json_error(struct json const *x) { return x->err; }

void json_reset(struct json *x)
{
  x->err = 0;
  cursor(x)->pos = 0;
  for (int i = 0; i <= get_parser(x)->size; ++i)
    nodes(x)[i].prev = 0;
}

int json_type(struct json const *x)
{
  return nodes((struct json *)x)[cursor((struct json *)x)->pos].type;
}

int json_nchild(struct json const *x) { return cnode((struct json *)x)->size; }

struct json *json_back(struct json *x)
{
  cursor(x)->pos = cnode(x)->prev;
  return x;
}

static struct json *setup_sentinel(struct json *x)
{
  nodes(x)[get_parser(x)->size].prev = cursor(x)->pos;
  cursor(x)->pos = get_parser(x)->size;
  return x;
}

struct json *json_down(struct json *x)
{
  if (json_type(x) == JSON_SENTINEL) return x;

  if (cnode(x)->size == 0) return setup_sentinel(x);

  return json_next(x);
}

struct json *json_next(struct json *x)
{
  if (json_type(x) == JSON_SENTINEL) return x;

  if (cursor(x)->pos + 1 >= get_parser(x)->size) return setup_sentinel(x);

  nodes(x)[cursor(x)->pos + 1].prev = cursor(x)->pos;
  cursor(x)->pos++;
  return x;
}

static struct json *rollback(struct json *x, int pos)
{
  while (cursor(json_back(x))->pos != pos)
    ;
  return x;
}

struct json *json_right(struct json *x)
{
  if (json_type(x) == JSON_SENTINEL) return x;

  int parent = cnode(x)->parent;
  int pos = cursor(x)->pos;
  if (parent == -1) return setup_sentinel(x);
  while (parent != cnode(json_next(x))->parent)
  {
    if (json_type(x) == JSON_SENTINEL)
    {
      setup_sentinel(rollback(x, pos));
      break;
    }
  }
  return x;
}

struct json *json_up(struct json *x)
{
  if (json_type(x) == JSON_SENTINEL) return x;

  int parent = cnode(x)->parent;
  if (parent == -1) return setup_sentinel(x);

  nodes(x)[parent].prev = cursor(x)->pos;
  cursor(x)->pos = parent;
  return x;
}

struct json *json_array_at(struct json *x, int idx)
{
  if (json_type(x) != JSON_ARRAY)
  {
    x->err = JSON_INVAL;
    return x;
  }

  int pos = cursor(x)->pos;
  json_down(x);
  for (int i = 0; i < idx; ++i)
  {
    if (json_type(x) == JSON_SENTINEL) break;
    json_right(x);
  }
  if (json_type(x) == JSON_SENTINEL)
  {
    rollback(x, pos);
    x->err = JSON_OUTRANGE;
  }
  return x;
}

struct json *json_object_at(struct json *x, char const *key)
{
  if (json_type(x) != JSON_OBJECT)
  {
    x->err = JSON_INVAL;
    return x;
  }

  int pos = cursor(x)->pos;
  json_down(x);
  while (strcmp(json_as_string(x), key))
  {
    if (json_type(x) == JSON_SENTINEL)
    {
      rollback(x, pos);
      x->err = JSON_NOTFOUND;
      return x;
    }
    json_right(x);
  }
  return json_down(x);
}

char *json_string_of(struct json *x, char const *key)
{
  if (json_type(x) != JSON_OBJECT) x->err = JSON_INVAL;
  if (json_error(x)) return empty_string(x);

  int pos = cursor(x)->pos;
  json_object_at(x, key);
  if (json_error(x)) return NULL;

  char *str = json_as_string(x);
  rollback(x, pos);
  return str;
}

void json_strcpy_of(struct json *x, char const *key, char *dst, int size)
{
  if (size > 0) dst[0] = '\0';

  if (json_type(x) != JSON_OBJECT) x->err = JSON_INVAL;
  if (json_error(x)) return;

  int pos = cursor(x)->pos;
  json_object_at(x, key);
  if (json_error(x)) return;

  char *str = json_as_string(x);
  if (__strlcpy(dst, str, size) >= size) x->err = JSON_NOMEM;
  rollback(x, pos);
}

bool json_bool_of(struct json *x, char const *key)
{
  if (json_type(x) != JSON_OBJECT) x->err = JSON_INVAL;
  if (json_error(x)) return false;

  int pos = cursor(x)->pos;
  json_object_at(x, key);
  if (json_error(x)) return false;

  bool val = json_as_bool(x);
  rollback(x, pos);
  return val;
}

void *json_null_of(struct json *x, char const *key)
{
  if (json_type(x) != JSON_OBJECT) x->err = JSON_INVAL;
  if (json_error(x)) return NULL;

  int pos = cursor(x)->pos;
  json_object_at(x, key);
  if (json_error(x)) return NULL;

  void *val = json_as_null(x);
  rollback(x, pos);
  return val;
}

long json_long_of(struct json *x, char const *key)
{
  if (json_type(x) != JSON_OBJECT) x->err = JSON_INVAL;
  if (json_error(x)) return 0;

  int pos = cursor(x)->pos;
  json_object_at(x, key);
  if (json_error(x)) return 0;

  long val = json_as_long(x);
  rollback(x, pos);
  return val;
}

unsigned long json_ulong_of(struct json *x, char const *key)
{
  if (json_type(x) != JSON_OBJECT) x->err = JSON_INVAL;
  if (json_error(x)) return 0;

  int pos = cursor(x)->pos;
  json_object_at(x, key);
  if (json_error(x)) return 0;

  unsigned long val = json_as_ulong(x);
  rollback(x, pos);
  return val;
}

double json_double_of(struct json *x, char const *key)
{
  if (json_type(x) != JSON_OBJECT) x->err = JSON_INVAL;
  if (json_error(x)) return 0.;

  int pos = cursor(x)->pos;
  json_object_at(x, key);
  if (json_error(x)) return 0.;

  double val = json_as_double(x);
  rollback(x, pos);
  return val;
}

char *json_as_string(struct json *x)
{
  if (json_type(x) != JSON_STRING) x->err = JSON_INVAL;
  if (json_error(x)) return empty_string(x);

  delimit(x);
  return cstring(x);
}

bool json_as_bool(struct json *x)
{
  if (json_type(x) != JSON_BOOL) x->err = JSON_INVAL;
  if (json_error(x)) return false;

  return cstring(x)[0] == 't';
}

void *json_as_null(struct json *x)
{
  if (json_type(x) != JSON_NULL) x->err = JSON_INVAL;
  return NULL;
}

long json_as_long(struct json *x)
{
  if (json_type(x) != JSON_NUMBER) x->err = JSON_INVAL;
  if (json_error(x)) return 0;

  delimit(x);
  long val = strto_long(cstring(x), NULL, 10);
  record_errno(x);
  return val;
}

unsigned long json_as_ulong(struct json *x)
{
  if (json_type(x) != JSON_NUMBER) x->err = JSON_INVAL;
  if (json_error(x)) return 0;

  delimit(x);
  unsigned long val = strto_ulong(cstring(x), NULL, 10);
  record_errno(x);
  return val;
}

double json_as_double(struct json *x)
{
  if (json_type(x) != JSON_NUMBER) x->err = JSON_INVAL;
  if (json_error(x)) return 0.;

  delimit(x);
  double val = strto_double(cstring(x), NULL);
  record_errno(x);
  return val;
}

static void sentinel_init(struct json *x)
{
  sentinel(x)->type = JSON_SENTINEL;
  sentinel(x)->start = 0;
  sentinel(x)->end = 1;
  sentinel(x)->size = 0;
  sentinel(x)->parent = get_parser(x)->size;
  sentinel(x)->prev = get_parser(x)->size;
}

static long strto_long(const char *restrict nptr, char **restrict endptr,
                       int base)
{
  errno = 0;
  intmax_t v = strtoimax(nptr, endptr, base);
  if (errno)
  {
    if (v == INTMAX_MAX) return LONG_MAX;
    if (v == INTMAX_MIN) return LONG_MIN;
    assert(v == 0);
    return 0;
  }
  if (v > LONG_MAX)
  {
    errno = ERANGE;
    return LONG_MAX;
  }
  if (v < LONG_MIN)
  {
    errno = ERANGE;
    return LONG_MIN;
  }
  return (long)v;
}

static unsigned long strto_ulong(const char *restrict nptr,
                                 char **restrict endptr, int base)
{
  errno = 0;
  uintmax_t v = strtoumax(nptr, endptr, base);
  if (errno)
  {
    if (v == UINTMAX_MAX) return ULONG_MAX;
    assert(v == 0);
    return 0;
  }
  if (v > ULONG_MAX)
  {
    errno = ERANGE;
    return ULONG_MAX;
  }
  return (unsigned long)v;
}

static double strto_double(const char *restrict nptr, char **restrict endptr)
{
  errno = 0;
  return strtod(nptr, endptr);
}

static int __strlcpy(char *dst, const char *src, int size)
{
  int ret = (int)strlen(src);

  if (size > 0)
  {
    int len = (ret >= size) ? size - 1 : ret;
    memcpy(dst, src, len);
    dst[len] = '\0';
  }
  return ret;
}

static void cursor_init(struct json_cursor *cursor, int length, char *json)
{
  cursor->length = length;
  cursor->json = json;
  cursor->pos = 0;
}

static char const *error_strings[] = {
    [JSON_INVAL] = "invalid value",
    [JSON_NOMEM] = "not enough memory",
    [JSON_OUTRANGE] = "out-of-range",
    [JSON_NOTFOUND] = "not found",
};

char const *json_strerror(int code)
{
  if (code < 0 || code >= (int)array_size(error_strings))
    return "unknown error";
  return error_strings[code];
}

struct json_node *__jr_node_alloc(struct json_parser *parser, int nnodes,
                                  struct json_node *nodes)
{
  if (parser->toknext >= nnodes) return NULL;
  struct json_node *node = &nodes[parser->toknext++];
  node->start = -1;
  node->end = -1;
  node->size = 0;
  node->parent = -1;
  return node;
}

static int parse_primitive(struct json_parser *parser, int length,
                           char const *json, int num_tokens,
                           struct json_node *tokens);
static int parse_string(struct json_parser *parser, int len, const char *js,
                        int nnodes, struct json_node *nodes);
static int primitive_type(char c);
static void fill_node(struct json_node *token, const int type, const int start,
                      const int end);
static int open_bracket(char c, struct json_parser *parser, int nnodes,
                        struct json_node *nodes);
static int check_umatched(struct json_parser *parser, struct json_node *nodes);
static int close_bracket(char c, struct json_parser *parser,
                         struct json_node *nodes);

static void parser_init(struct json_parser *parser, int alloc_size)
{
  parser->alloc_size = alloc_size;
  parser->size = 0;
  parser->pos = 0;
  parser->toknext = 0;
  parser->toksuper = -1;
}

static void parser_reset(struct json_parser *parser)
{
  parser->size = 0;
  parser->pos = 0;
  parser->toknext = 0;
  parser->toksuper = -1;
}

static int parser_parse(struct json_parser *parser, const int len, char *js,
                        int nnodes, struct json_node *nodes)
{
  int rc = 0;
  parser->size = parser->toknext;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
  {
    char c = js[parser->pos];
    switch (c)
    {
    case '{':
    case '[':
      parser->size++;
      if ((rc = open_bracket(c, parser, nnodes, nodes))) return rc;
      break;
    case '}':
    case ']':
      if ((rc = close_bracket(c, parser, nodes))) return rc;
      break;
    case '\"':
      if ((rc = parse_string(parser, len, js, nnodes, nodes))) return rc;
      parser->size++;
      if (parser->toksuper != -1 && nodes != NULL)
      {
        nodes[parser->toksuper].size++;
      }
      break;
    case '\t':
    case '\r':
    case '\n':
    case ' ':
      break;
    case ':':
      parser->toksuper = parser->toknext - 1;
      break;
    case ',':
      if (parser->toksuper != -1 &&
          nodes[parser->toksuper].type != JSON_ARRAY &&
          nodes[parser->toksuper].type != JSON_OBJECT)
      {
        parser->toksuper = nodes[parser->toksuper].parent;
      }
      break;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 't':
    case 'f':
    case 'n':
      /* And they must not be keys of the object */
      if (parser->toksuper != -1)
      {
        const struct json_node *t = &nodes[parser->toksuper];
        if (t->type == JSON_OBJECT || (t->type == JSON_STRING && t->size != 0))
        {
          return JSON_INVAL;
        }
      }
      if ((rc = parse_primitive(parser, len, js, nnodes, nodes))) return rc;
      parser->size++;
      if (parser->toksuper != -1)
      {
        nodes[parser->toksuper].size++;
      }
      break;

    /* Unexpected char in strict mode */
    default:
      return JSON_INVAL;
    }
  }

  if ((rc = check_umatched(parser, nodes))) return rc;

  return 0;
}

static int parse_primitive(struct json_parser *parser, int len, char const *js,
                           int nnodes, struct json_node *nodes)
{
  int start = parser->pos;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
  {
    switch (js[parser->pos])
    {
    case '\t':
    case '\r':
    case '\n':
    case ' ':
    case ',':
    case ']':
    case '}':
      goto found;
    default:
      /* to quiet a warning from gcc*/
      break;
    }
    if (js[parser->pos] < 32 || js[parser->pos] >= 127)
    {
      parser->pos = start;
      return JSON_INVAL;
    }
  }
  /* In strict mode primitive must be followed by a comma/object/array */
  parser->pos = start;
  return JSON_INVAL;

found:;
  struct json_node *node = __jr_node_alloc(parser, nnodes, nodes);
  if (node == NULL)
  {
    parser->pos = start;
    return JSON_NOMEM;
  }
  fill_node(node, primitive_type(js[start]), start, parser->pos);
  node->parent = parser->toksuper;
  parser->pos--;
  return 0;
}

static int parse_string(struct json_parser *parser, int len, const char *js,
                        int nnodes, struct json_node *nodes)
{
  struct json_node *token;

  int start = parser->pos;

  /* Skip starting quote */
  parser->pos++;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
  {
    char c = js[parser->pos];

    /* Quote: end of string */
    if (c == '\"')
    {
      if (nodes == NULL)
      {
        return 0;
      }
      token = __jr_node_alloc(parser, nnodes, nodes);
      if (token == NULL)
      {
        parser->pos = start;
        return JSON_NOMEM;
      }
      fill_node(token, JSON_STRING, start + 1, parser->pos);
      token->parent = parser->toksuper;
      return 0;
    }

    /* Backslash: Quoted symbol expected */
    if (c == '\\' && parser->pos + 1 < len)
    {
      int i;
      parser->pos++;
      switch (js[parser->pos])
      {
      /* Allowed escaped symbols */
      case '\"':
      case '/':
      case '\\':
      case 'b':
      case 'f':
      case 'r':
      case 'n':
      case 't':
        break;
      /* Allows escaped symbol \uXXXX */
      case 'u':
        parser->pos++;
        for (i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++)
        {
          /* If it isn't a hex character we have an error */
          if (!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
                (js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
                (js[parser->pos] >= 97 && js[parser->pos] <= 102)))
          { /* a-f */
            parser->pos = start;
            return JSON_INVAL;
          }
          parser->pos++;
        }
        parser->pos--;
        break;
      /* Unexpected symbol */
      default:
        parser->pos = start;
        return JSON_INVAL;
      }
    }
  }
  parser->pos = start;
  return JSON_INVAL;
}

static int primitive_type(char c)
{
  switch (c)
  {
  case '-':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    return JSON_NUMBER;
  case 't':
  case 'f':
    return JSON_BOOL;
    break;
  case 'n':
    return JSON_NULL;
    break;
  default:
    assert(false);
  }
  assert(false);
  return 0;
}

static void fill_node(struct json_node *token, const int type, const int start,
                      const int end)
{
  token->type = type;
  token->start = start;
  token->end = end;
  token->size = 0;
}

static int open_bracket(char c, struct json_parser *parser, int nnodes,
                        struct json_node *nodes)
{
  struct json_node *node = __jr_node_alloc(parser, nnodes, nodes);
  if (node == NULL) return JSON_NOMEM;
  if (parser->toksuper != -1)
  {
    struct json_node *t = &nodes[parser->toksuper];
    /* In strict mode an object or array can't become a key */
    if (t->type == JSON_OBJECT)
    {
      return JSON_INVAL;
    }
    t->size++;
    node->parent = parser->toksuper;
  }
  node->type = (c == '{' ? JSON_OBJECT : JSON_ARRAY);
  node->start = parser->pos;
  parser->toksuper = parser->toknext - 1;
  return 0;
}

static int check_umatched(struct json_parser *parser, struct json_node *nodes)
{
  for (int i = parser->toknext - 1; i >= 0; i--)
  {
    /* Unmatched opened object or array */
    if (nodes[i].start != -1 && nodes[i].end == -1) return JSON_INVAL;
  }
  return 0;
}

static int close_bracket(char c, struct json_parser *parser,
                         struct json_node *nodes)
{
  int type = (c == '}' ? JSON_OBJECT : JSON_ARRAY);
  if (parser->toknext < 1)
  {
    return JSON_INVAL;
  }
  struct json_node *node = &nodes[parser->toknext - 1];
  for (;;)
  {
    if (node->start != -1 && node->end == -1)
    {
      if (node->type != type)
      {
        return JSON_INVAL;
      }
      node->end = parser->pos + 1;
      parser->toksuper = node->parent;
      break;
    }
    if (node->parent == -1)
    {
      if (node->type != type || parser->toksuper == -1)
      {
        return JSON_INVAL;
      }
      break;
    }
    node = &nodes[node->parent];
  }
  return 0;
}
