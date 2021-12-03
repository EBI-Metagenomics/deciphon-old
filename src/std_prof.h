#ifndef STD_PROF_H
#define STD_PROF_H

#include "prof.h"
#include "rc.h"
#include <stdio.h>

struct dcp_std_prof
{
    struct dcp_prof super;
    struct
    {
        struct imm_dp null;
        struct imm_dp alt;
    } dp;
};

void dcp_std_prof_init(struct dcp_std_prof *, struct imm_code const *);

static inline void dcp_std_prof_del(struct dcp_std_prof *prof)
{
    if (prof) prof->super.vtable.del(&prof->super);
}

static inline struct dcp_prof *dcp_std_prof_super(struct dcp_std_prof *std)
{
    return &std->super;
}

struct dcp_std_prof;

enum dcp_rc std_prof_read(struct dcp_std_prof *prof, FILE *restrict fd);
enum dcp_rc std_prof_write(struct dcp_std_prof const *prof, FILE *restrict fd);

#endif
