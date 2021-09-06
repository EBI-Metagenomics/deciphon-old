#include "dcp/std_profile.h"
#include "dcp/rc.h"
#include "imm/imm.h"
#include "profile.h"
#include "std_prof.h"

static void del(struct dcp_prof *prof);

void dcp_std_prof_init(struct dcp_std_prof *prof, struct imm_abc const *abc)
{
    imm_dp_init(&prof->dp.null, abc);
    imm_dp_init(&prof->dp.alt, abc);
    struct dcp_prof_vtable vtable = {std_prof_read, std_prof_write, del,
                                     DCP_STD_PROFILE, prof};
    profile_init(&prof->super, abc, dcp_meta(NULL, NULL), vtable);
}

enum dcp_rc std_prof_read(struct dcp_prof *prof, FILE *restrict fd)
{
    struct dcp_std_prof *p = prof->vtable.derived;

    if (imm_dp_read(&p->dp.null, fd)) return DCP_RUNTIMEERROR;

    if (imm_dp_read(&p->dp.alt, fd)) return DCP_RUNTIMEERROR;

    return DCP_SUCCESS;
}

enum dcp_rc std_prof_write(struct dcp_prof const *prof, FILE *restrict fd)
{
    struct dcp_std_prof const *p = prof->vtable.derived;

    if (imm_dp_write(&p->dp.null, fd)) return DCP_RUNTIMEERROR;

    if (imm_dp_write(&p->dp.alt, fd)) return DCP_RUNTIMEERROR;

    return DCP_SUCCESS;
}

static void del(struct dcp_prof *prof)
{
    if (prof)
    {
        struct dcp_std_prof *p = prof->vtable.derived;
        imm_del(&p->dp.null);
        imm_del(&p->dp.alt);
    }
}
