#ifndef META_H
#define META_H

struct dcp_meta
{
    char const *name;
    char const *acc;
};

static inline struct dcp_meta dcp_meta(char const *name, char const *acc)
{
    return (struct dcp_meta){name, acc};
}

extern struct dcp_meta meta_unset;

#endif
