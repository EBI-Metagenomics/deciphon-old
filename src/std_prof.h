#ifndef STD_PROF_H
#define STD_PROF_H

#include "prof.h"
#include "rc.h"
#include <stdio.h>

struct std_prof
{
    struct dcp_prof super;
    struct
    {
        struct imm_dp null;
        struct imm_dp alt;
    } dp;
};

void std_prof_init(struct std_prof *, struct imm_code const *);

static inline void dcp_std_prof_del(struct std_prof *prof)
{
    if (prof) prof->super.vtable.del(&prof->super);
}

static inline struct dcp_prof *dcp_std_prof_super(struct std_prof *std)
{
    return &std->super;
}

struct std_prof;

enum rc std_prof_read(struct std_prof *prof, FILE *restrict fd);
enum rc std_prof_write(struct std_prof const *prof, FILE *restrict fd);

static inline void std_prof_del(struct std_prof *prof)
{
    dcp_prof_del(&prof->super);
}

#endif
