#ifndef METADATA_H
#define METADATA_H

struct metadata
{
    char const *name;
    char const *acc;
};

static inline struct metadata metadata_unset(void)
{
    return (struct metadata){"UNSET", "UNSET"};
}

static inline struct metadata metadata(char const *name, char const *acc)
{
    return (struct metadata){name, acc};
}

#endif
