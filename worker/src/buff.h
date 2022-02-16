#ifndef BUFF_H
#define BUFF_H

#include <stdbool.h>
#include <stddef.h>

struct buff
{
    size_t size;
    size_t capacity;
    char data[];
};

struct buff *buff_new(size_t size);
struct buff *buff_ensure(struct buff *buff, size_t size);
// struct buff *buff_compact(struct buff *buff, size_t size);
void buff_del(struct buff const *buff);

#endif
