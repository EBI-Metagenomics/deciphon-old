#ifndef DECIPHON_LJSON_H
#define DECIPHON_LJSON_H

struct ljson_ctx
{
    unsigned size;
    char *buf;
    char *cur;
    unsigned elem;
    int error;
    char tmpbuf[20];
};

void ljson_open(struct ljson_ctx *, unsigned size, char *str);
void ljson_close(struct ljson_ctx *);
int ljson_error(struct ljson_ctx *);

void ljson_str(struct ljson_ctx *, char *key, char const *str);
void ljson_int(struct ljson_ctx *, char *key, long value);
void ljson_bool(struct ljson_ctx *, char *key, int value);
void ljson_null(struct ljson_ctx *, char *key);

#endif
