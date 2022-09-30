#include "jx.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Source: https://stackoverflow.com/a/18298965
#ifndef thread_local
#if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#define thread_local _Thread_local
#elif defined _WIN32 && (defined _MSC_VER || defined __ICL ||                  \
                         defined __DMC__ || defined __BORLANDC__)
#define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
#elif defined __GNUC__ || defined __SUNPRO_C || defined __xlC__
#define thread_local __thread
#else
#error "Cannot define thread_local"
#endif
#endif

static thread_local int error = JR_OK;

enum offset
{
    PARSER_OFFSET = 0,
    CURSOR_OFFSET = 1,
    NODE_OFFSET = 2,
};

static inline struct jr_parser *get_parser(struct jr jr[])
{
    return &jr[PARSER_OFFSET].parser;
}
static inline struct jr_cursor *cursor(struct jr jr[])
{
    return &jr[CURSOR_OFFSET].cursor;
}
static inline struct jr_node *nodes(struct jr jr[])
{
    return &jr[NODE_OFFSET].node;
}
static inline struct jr_node *cnode(struct jr jr[])
{
    return nodes(jr) + cursor(jr)->pos;
}
static inline struct jr_node *sentinel(struct jr jr[])
{
    return &nodes(jr)[get_parser(jr)->size];
}
static inline void delimit(struct jr jr[])
{
    cursor(jr)->json[cnode(jr)->end] = '\0';
}
static inline void input_errno(void)
{
    if (errno == EINVAL) error = JR_INVAL;
    if (errno == ERANGE) error = JR_OUTRANGE;
}
static inline char *cstring(struct jr jr[])
{
    return &cursor(jr)->json[cnode(jr)->start];
}
static inline char *empty_string(struct jr jr[])
{
    return &cursor(jr)->json[cursor(jr)->length];
}
static void sentinel_init(struct jr jr[]);
static long strto_long(const char *restrict, char **restrict, int);
static unsigned long strto_ulong(const char *restrict, char **restrict, int);
static double strto_double(const char *restrict, char **restrict);
static int jr_strlcpy(char *dst, const char *src, int size);
static void jr_parser_init(struct jr_parser *parser, int size);
static void jr_parser_reset(struct jr_parser *parser);
static int jr_parser_parse(struct jr_parser *, int length, char *json,
                           int nnodes, struct jr_node *);
static void jr_cursor_init(struct jr_cursor *cursor, int length, char *json);

void __jr_init(struct jr jr[], int alloc_size)
{
    error = JR_OK;
    jr_parser_init(get_parser(jr), alloc_size);
}

int jr_parse(struct jr jr[], int length, char *json)
{
    error = JR_OK;
    jr_parser_reset(get_parser(jr));
    jr_cursor_init(cursor(jr), length, json);
    struct jr_parser *p = get_parser(jr);
    struct jr_cursor *c = cursor(jr);
    int n = p->alloc_size - 2;
    error = jr_parser_parse(p, c->length, c->json, n, nodes(jr));
    if (error) return error;
    sentinel_init(jr);
    if (p->size > 0) cnode(jr)->parent = -1;
    return error;
}

int jr_error(void) { return error; }

void jr_reset(struct jr jr[])
{
    error = JR_OK;
    cursor(jr)->pos = 0;
    for (int i = 0; i <= get_parser(jr)->size; ++i)
        nodes(jr)[i].prev = 0;
}

int jr_type(struct jr const jr[])
{
    return nodes((struct jr *)jr)[cursor((struct jr *)jr)->pos].type;
}

int jr_nchild(struct jr const jr[]) { return cnode((struct jr *)jr)->size; }

struct jr *jr_back(struct jr jr[])
{
    cursor(jr)->pos = cnode(jr)->prev;
    return jr;
}

static struct jr *setup_sentinel(struct jr jr[])
{
    nodes(jr)[get_parser(jr)->size].prev = cursor(jr)->pos;
    cursor(jr)->pos = get_parser(jr)->size;
    return jr;
}

struct jr *jr_down(struct jr jr[])
{
    if (jr_type(jr) == JR_SENTINEL) return jr;

    if (cnode(jr)->size == 0) return setup_sentinel(jr);

    return jr_next(jr);
}

struct jr *jr_next(struct jr jr[])
{
    if (jr_type(jr) == JR_SENTINEL) return jr;

    if (cursor(jr)->pos + 1 >= get_parser(jr)->size) return setup_sentinel(jr);

    nodes(jr)[cursor(jr)->pos + 1].prev = cursor(jr)->pos;
    cursor(jr)->pos++;
    return jr;
}

static struct jr *rollback(struct jr jr[], int pos)
{
    while (cursor(jr_back(jr))->pos != pos)
        ;
    return jr;
}

struct jr *jr_right(struct jr jr[])
{
    if (jr_type(jr) == JR_SENTINEL) return jr;

    int parent = cnode(jr)->parent;
    int pos = cursor(jr)->pos;
    if (parent == -1) return setup_sentinel(jr);
    while (parent != cnode(jr_next(jr))->parent)
    {
        if (jr_type(jr) == JR_SENTINEL)
        {
            setup_sentinel(rollback(jr, pos));
            break;
        }
    }
    return jr;
}

struct jr *jr_up(struct jr jr[])
{
    if (jr_type(jr) == JR_SENTINEL) return jr;

    int parent = cnode(jr)->parent;
    if (parent == -1) return setup_sentinel(jr);

    nodes(jr)[parent].prev = cursor(jr)->pos;
    cursor(jr)->pos = parent;
    return jr;
}

struct jr *jr_array_at(struct jr jr[], int idx)
{
    if (jr_type(jr) != JR_ARRAY)
    {
        error = JR_INVAL;
        return jr;
    }

    int pos = cursor(jr)->pos;
    jr_down(jr);
    for (int i = 0; i < idx; ++i)
    {
        if (jr_type(jr) == JR_SENTINEL) break;
        jr_right(jr);
    }
    if (jr_type(jr) == JR_SENTINEL)
    {
        rollback(jr, pos);
        error = JR_OUTRANGE;
    }
    return jr;
}

struct jr *jr_object_at(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT)
    {
        error = JR_INVAL;
        return jr;
    }

    int pos = cursor(jr)->pos;
    jr_down(jr);
    while (strcmp(jr_as_string(jr), key))
    {
        if (jr_type(jr) == JR_SENTINEL)
        {
            rollback(jr, pos);
            error = JR_NOTFOUND;
            return jr;
        }
        jr_right(jr);
    }
    return jr_down(jr);
}

char *jr_string_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return empty_string(jr);

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    char *str = jr_as_string(jr);
    rollback(jr, pos);
    return str;
}

void jr_strcpy_of(struct jr jr[], char const *key, char *dst, int size)
{
    if (size > 0) dst[0] = '\0';

    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    char *str = jr_as_string(jr);
    if (jr_strlcpy(dst, str, size) >= size) error = JR_NOMEM;
    rollback(jr, pos);
}

bool jr_bool_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return 0;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    bool val = jr_as_bool(jr);
    rollback(jr, pos);
    return val;
}

void *jr_null_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return 0;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    void *val = jr_as_null(jr);
    rollback(jr, pos);
    return val;
}

long jr_long_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return 0;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    long val = jr_as_long(jr);
    rollback(jr, pos);
    return val;
}

unsigned long jr_ulong_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return 0;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    unsigned long val = jr_as_ulong(jr);
    rollback(jr, pos);
    return val;
}

double jr_double_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return 0;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    double val = jr_as_double(jr);
    rollback(jr, pos);
    return val;
}

char *jr_as_string(struct jr jr[])
{
    if (jr_type(jr) != JR_STRING) error = JR_INVAL;
    if (error) return empty_string(jr);

    delimit(jr);
    return cstring(jr);
}

bool jr_as_bool(struct jr jr[])
{
    if (jr_type(jr) != JR_BOOL) error = JR_INVAL;
    if (error) return false;

    return cstring(jr)[0] == 't';
}

void *jr_as_null(struct jr jr[])
{
    if (jr_type(jr) != JR_NULL) error = JR_INVAL;
    return NULL;
}

long jr_as_long(struct jr jr[])
{
    if (jr_type(jr) != JR_NUMBER) error = JR_INVAL;
    if (error) return 0;

    delimit(jr);
    long val = strto_long(cstring(jr), NULL, 10);
    input_errno();
    return val;
}

unsigned long jr_as_ulong(struct jr jr[])
{
    if (jr_type(jr) != JR_NUMBER) error = JR_INVAL;
    if (error) return 0;

    delimit(jr);
    unsigned long val = strto_ulong(cstring(jr), NULL, 10);
    input_errno();
    return val;
}

double jr_as_double(struct jr jr[])
{
    if (jr_type(jr) != JR_NUMBER) error = JR_INVAL;
    if (error) return 0;

    delimit(jr);
    double val = strto_double(cstring(jr), NULL);
    input_errno();
    return val;
}

static void sentinel_init(struct jr jr[])
{
    sentinel(jr)->type = JR_SENTINEL;
    sentinel(jr)->start = 0;
    sentinel(jr)->end = 1;
    sentinel(jr)->size = 0;
    sentinel(jr)->parent = get_parser(jr)->size;
    sentinel(jr)->prev = get_parser(jr)->size;
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

static int jr_strlcpy(char *dst, const char *src, int size)
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

static void jr_cursor_init(struct jr_cursor *cursor, int length, char *json)
{
    cursor->length = length;
    cursor->json = json;
    cursor->pos = 0;
}

static char const *error_strings[] = {
#define X(_, A) A,
    JR_ERROR_MAP(X)
#undef X
};

char const *jr_strerror(int code)
{
    if (code < 0 || code >= (int)__JR_ARRAY_SIZE(error_strings))
        return "unknown error";
    return error_strings[code];
}

struct jr_node *__jr_node_alloc(struct jr_parser *parser, int nnodes,
                                struct jr_node *nodes)
{
    if (parser->toknext >= nnodes) return NULL;
    struct jr_node *node = &nodes[parser->toknext++];
    node->start = -1;
    node->end = -1;
    node->size = 0;
    node->parent = -1;
    return node;
}

#include <assert.h>
#include <stdbool.h>

static int parse_primitive(struct jr_parser *parser, int length,
                           char const *json, int num_tokens,
                           struct jr_node *tokens);
static int parse_string(struct jr_parser *parser, int len, const char *js,
                        int nnodes, struct jr_node *nodes);
static int primitive_type(char c);
static void fill_node(struct jr_node *token, const int type, const int start,
                      const int end);
static int open_bracket(char c, struct jr_parser *parser, int nnodes,
                        struct jr_node *nodes);
static int check_umatched(struct jr_parser *parser, struct jr_node *nodes);
static int close_bracket(char c, struct jr_parser *parser,
                         struct jr_node *nodes);

static void jr_parser_init(struct jr_parser *parser, int alloc_size)
{
    parser->alloc_size = alloc_size;
    parser->size = 0;
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

static void jr_parser_reset(struct jr_parser *parser)
{
    parser->size = 0;
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

static int jr_parser_parse(struct jr_parser *parser, const int len, char *js,
                           int nnodes, struct jr_node *nodes)
{
    int rc = JR_OK;
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
                nodes[parser->toksuper].type != JR_ARRAY &&
                nodes[parser->toksuper].type != JR_OBJECT)
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
                const struct jr_node *t = &nodes[parser->toksuper];
                if (t->type == JR_OBJECT ||
                    (t->type == JR_STRING && t->size != 0))
                {
                    return JR_INVAL;
                }
            }
            if ((rc = parse_primitive(parser, len, js, nnodes, nodes)))
                return rc;
            parser->size++;
            if (parser->toksuper != -1)
            {
                nodes[parser->toksuper].size++;
            }
            break;

        /* Unexpected char in strict mode */
        default:
            return JR_INVAL;
        }
    }

    if ((rc = check_umatched(parser, nodes))) return rc;

    return JR_OK;
}

static int parse_primitive(struct jr_parser *parser, int len, char const *js,
                           int nnodes, struct jr_node *nodes)
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
            return JR_INVAL;
        }
    }
    /* In strict mode primitive must be followed by a comma/object/array */
    parser->pos = start;
    return JR_INVAL;

found:;
    struct jr_node *node = __jr_node_alloc(parser, nnodes, nodes);
    if (node == NULL)
    {
        parser->pos = start;
        return JR_NOMEM;
    }
    fill_node(node, primitive_type(js[start]), start, parser->pos);
    node->parent = parser->toksuper;
    parser->pos--;
    return JR_OK;
}

static int parse_string(struct jr_parser *parser, int len, const char *js,
                        int nnodes, struct jr_node *nodes)
{
    struct jr_node *token;

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
                return JR_OK;
            }
            token = __jr_node_alloc(parser, nnodes, nodes);
            if (token == NULL)
            {
                parser->pos = start;
                return JR_NOMEM;
            }
            fill_node(token, JR_STRING, start + 1, parser->pos);
            token->parent = parser->toksuper;
            return JR_OK;
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
                for (i = 0;
                     i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++)
                {
                    /* If it isn't a hex character we have an error */
                    if (!((js[parser->pos] >= 48 &&
                           js[parser->pos] <= 57) || /* 0-9 */
                          (js[parser->pos] >= 65 &&
                           js[parser->pos] <= 70) || /* A-F */
                          (js[parser->pos] >= 97 && js[parser->pos] <= 102)))
                    { /* a-f */
                        parser->pos = start;
                        return JR_INVAL;
                    }
                    parser->pos++;
                }
                parser->pos--;
                break;
            /* Unexpected symbol */
            default:
                parser->pos = start;
                return JR_INVAL;
            }
        }
    }
    parser->pos = start;
    return JR_INVAL;
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
        return JR_NUMBER;
    case 't':
    case 'f':
        return JR_BOOL;
        break;
    case 'n':
        return JR_NULL;
        break;
    default:
        assert(false);
    }
    assert(false);
    return 0;
}

static void fill_node(struct jr_node *token, const int type, const int start,
                      const int end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

static int open_bracket(char c, struct jr_parser *parser, int nnodes,
                        struct jr_node *nodes)
{
    struct jr_node *node = __jr_node_alloc(parser, nnodes, nodes);
    if (node == NULL) return JR_NOMEM;
    if (parser->toksuper != -1)
    {
        struct jr_node *t = &nodes[parser->toksuper];
        /* In strict mode an object or array can't become a key */
        if (t->type == JR_OBJECT)
        {
            return JR_INVAL;
        }
        t->size++;
        node->parent = parser->toksuper;
    }
    node->type = (c == '{' ? JR_OBJECT : JR_ARRAY);
    node->start = parser->pos;
    parser->toksuper = parser->toknext - 1;
    return JR_OK;
}

static int check_umatched(struct jr_parser *parser, struct jr_node *nodes)
{
    for (int i = parser->toknext - 1; i >= 0; i--)
    {
        /* Unmatched opened object or array */
        if (nodes[i].start != -1 && nodes[i].end == -1) return JR_INVAL;
    }
    return JR_OK;
}

static int close_bracket(char c, struct jr_parser *parser,
                         struct jr_node *nodes)
{
    int type = (c == '}' ? JR_OBJECT : JR_ARRAY);
    if (parser->toknext < 1)
    {
        return JR_INVAL;
    }
    struct jr_node *node = &nodes[parser->toknext - 1];
    for (;;)
    {
        if (node->start != -1 && node->end == -1)
        {
            if (node->type != type)
            {
                return JR_INVAL;
            }
            node->end = parser->pos + 1;
            parser->toksuper = node->parent;
            break;
        }
        if (node->parent == -1)
        {
            if (node->type != type || parser->toksuper == -1)
            {
                return JR_INVAL;
            }
            break;
        }
        node = &nodes[node->parent];
    }
    return JR_OK;
}

static unsigned put_unquoted_cstr(char buf[], char const *str);
static unsigned itoa_rev(char buf[], long i);
static unsigned utoa_rev(char buf[], unsigned long i);
static void reverse(char buf[], int size);

unsigned jw_bool(char buf[], bool x)
{
    return put_unquoted_cstr(buf, x ? "true" : "false");
}

unsigned jw_long(char buf[], long x)
{
    unsigned size = itoa_rev(buf, x);
    reverse(buf, size);
    return size;
}

unsigned jw_ulong(char buf[], unsigned long x)
{
    unsigned size = utoa_rev(buf, x);
    reverse(buf, size);
    return size;
}

unsigned jw_null(char buf[]) { return put_unquoted_cstr(buf, "null"); }

unsigned jw_string(char buf[], char const x[])
{
    buf[0] = '\"';
    unsigned size = put_unquoted_cstr(buf + 1, x);
    buf[size + 1] = '\"';
    return size + 2;
}

unsigned jw_object_open(char buf[])
{
    buf[0] = '{';
    return 1;
}

unsigned jw_object_close(char buf[])
{
    buf[0] = '}';
    return 1;
}

unsigned jw_array_open(char buf[])
{
    buf[0] = '[';
    return 1;
}

unsigned jw_array_close(char buf[])
{
    buf[0] = ']';
    return 1;
}

unsigned jw_comma(char buf[])
{
    buf[0] = ',';
    return 1;
}

unsigned jw_colon(char buf[])
{
    buf[0] = ':';
    return 1;
}

static unsigned put_unquoted_cstr(char buf[], char const *cstr)
{
    char *p = buf;
    while (*cstr)
        *p++ = *cstr++;
    return (unsigned)(p - buf);
}

static unsigned itoa_rev(char buf[], long i)
{
    char *dst = buf;
    if (i == 0)
    {
        *dst++ = '0';
    }
    int neg = (i < 0) ? -1 : 1;
    while (i)
    {
        *dst++ = (char)('0' + (neg * (i % 10)));
        i /= 10;
    }
    if (neg == -1)
    {
        *dst++ = '-';
    }
    return (unsigned)(dst - buf);
}

static unsigned utoa_rev(char buf[], unsigned long i)
{
    char *dst = buf;
    if (i == 0)
    {
        *dst++ = '0';
    }
    while (i)
    {
        *dst++ = (char)('0' + (i % 10));
        i /= 10;
    }
    return (unsigned)(dst - buf);
}

#define XOR_SWAP(a, b)                                                         \
    do                                                                         \
    {                                                                          \
        a ^= b;                                                                \
        b ^= a;                                                                \
        a ^= b;                                                                \
    } while (0)

static void reverse(char buf[], int size)
{
    char *end = buf + size - 1;

    while (buf < end)
    {
        XOR_SWAP(*buf, *end);
        buf++;
        end--;
    }
}
