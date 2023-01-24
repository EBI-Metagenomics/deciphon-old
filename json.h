#ifndef JSON_H
#define JSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum json_type
{
  JSON_SENTINEL = 0,
  JSON_OBJECT = 1,
  JSON_ARRAY = 2,
  JSON_STRING = 3,
  JSON_NULL = 4,
  JSON_BOOL = 5,
  JSON_NUMBER = 6,
};

enum json_error
{
  JSON_INVAL = 1,
  JSON_NOMEM = 2,
  JSON_OUTRANGE = 3,
  JSON_NOTFOUND = 4,
};

struct json_node
{
  int type;
  int start;
  int end;
  int size;
  int parent;
  int prev;
};

struct json_parser
{
  int alloc_size;
  int size;
  int pos;
  int toknext;
  int toksuper;
};

struct json_cursor
{
  int length;
  char *json;
  int pos;
};

struct json
{
  int err;
  union
  {
    struct json_parser parser;
    struct json_cursor cursor;
    struct json_node node;
  };
};

void json_init(struct json *, int alloc_size);
int json_parse(struct json *, int length, char *json);
int json_error(struct json const *);
char const *json_strerror(int code);
void json_reset(struct json *);
int json_type(struct json const *);
int json_nchild(struct json const *);

struct json *json_back(struct json *);
struct json *json_down(struct json *);
struct json *json_next(struct json *);
struct json *json_right(struct json *);
struct json *json_up(struct json *);

struct json *json_array_at(struct json *, int idx);
struct json *json_object_at(struct json *, char const *key);

char *json_string_of(struct json *, char const *key);
void json_strcpy_of(struct json *, char const *key, char *dst, int size);
bool json_bool_of(struct json *, char const *key);
void *json_null_of(struct json *, char const *key);
long json_long_of(struct json *, char const *key);
unsigned long json_ulong_of(struct json *, char const *key);
double json_double_of(struct json *, char const *key);

char *json_as_string(struct json *);
bool json_as_bool(struct json *);
void *json_as_null(struct json *);
long json_as_long(struct json *);
unsigned long json_as_ulong(struct json *);
double json_as_double(struct json *);

#endif
