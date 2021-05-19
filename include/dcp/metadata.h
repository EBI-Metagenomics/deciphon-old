#ifndef DCP_METADATA_H
#define DCP_METADATA_H

struct dcp_metadata
{
    char const *name;
    char const *acc;
};

static inline struct dcp_metadata dcp_metadata(char const *name,
                                               char const *acc)
{
    return (struct dcp_metadata){name, acc};
}

#endif
