#ifndef JX_H
#define JX_H

#ifdef NULL
#undef NULL
#define NULL ((void *)0)
#endif

enum jr_type
{
    JR_SENTINEL = 0,
    JR_OBJECT = 1,
    JR_ARRAY = 2,
    JR_STRING = 3,
    JR_NULL = 4,
    JR_BOOL = 5,
    JR_NUMBER = 6,
};

enum jr_error
{
    JR_OK,
    JR_INVAL,
    JR_NOMEM,
    JR_OUTRANGE,
    JR_NOTFOUND,
};

struct jr_node
{
    int type;
    int start;
    int end;
    int size;
    int parent;
    int prev;
};

struct jr_parser
{
    int alloc_size;
    int size;
    int pos;
    int toknext;
    int toksuper;
};

struct jr_cursor
{
    int length;
    char *json;
    int pos;
};

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct jr
{
    union
    {
        struct jr_parser parser;
        struct jr_cursor cursor;
        struct jr_node node;
    };
};

#define __JR_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define JR_DECLARE(name, size) struct jr name[size];
#define JR_INIT(name) __jr_init((name), __JR_ARRAY_SIZE(name))

void __jr_init(struct jr[], int alloc_size);
int jr_parse(struct jr[], int length, char *json);
int jr_error(void);
void jr_reset(struct jr[]);
int jr_type(struct jr const[]);
int jr_nchild(struct jr const[]);

struct jr *jr_back(struct jr[]);
struct jr *jr_down(struct jr[]);
struct jr *jr_next(struct jr[]);
struct jr *jr_right(struct jr[]);
struct jr *jr_up(struct jr[]);

struct jr *jr_array_at(struct jr[], int idx);
struct jr *jr_object_at(struct jr[], char const *key);

char *jr_string_of(struct jr[], char const *key);
void jr_strcpy_of(struct jr[], char const *key, char *dst, int size);
long jr_long_of(struct jr[], char const *key);
unsigned long jr_ulong_of(struct jr[], char const *key);

char *jr_as_string(struct jr[]);
bool jr_as_bool(struct jr[]);
void *jr_as_null(struct jr[]);
long jr_as_long(struct jr[]);
unsigned long jr_as_ulong(struct jr[]);
double jr_as_double(struct jr[]);

#include <stdbool.h>

unsigned jw_bool(char buf[], bool x);
unsigned jw_long(char buf[], long x);
unsigned jw_ulong(char buf[], unsigned long x);
unsigned jw_null(char buf[]);
unsigned jw_string(char buf[], char const x[]);

unsigned jw_object_open(char buf[]);
unsigned jw_object_close(char buf[]);

unsigned jw_array_open(char buf[]);
unsigned jw_array_close(char buf[]);

unsigned jw_comma(char buf[]);
unsigned jw_colon(char buf[]);

#endif
