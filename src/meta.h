#ifndef META_H
#define META_H

struct meta
{
    char const *name;
    char const *acc;
};

static inline struct meta meta(char const *name, char const *acc)
{
    return (struct meta){name, acc};
}

extern struct meta meta_unset;

#endif
