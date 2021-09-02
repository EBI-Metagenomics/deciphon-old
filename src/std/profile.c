#include "profile.h"
#include "dcp/rc.h"
#include "dcp/std/profile.h"
#include "imm/imm.h"
#include "support.h"

static enum dcp_rc read(struct dcp_profile *prof, FILE *restrict fd);

static enum dcp_rc write(struct dcp_profile const *prof, FILE *restrict fd);

static void del(struct dcp_profile *prof);

DCP_API void dcp_std_profile_init(struct dcp_std_profile *p,
                                  struct imm_abc const *abc)
{
    imm_dp_init(&p->dp.null, abc);
    imm_dp_init(&p->dp.alt, abc);
    struct dcp_profile_vtable vtable = {read, write, del, DCP_STD_PROFILE, p};
    profile_init(&p->super, abc, dcp_meta(NULL, NULL), vtable);
}

static enum dcp_rc read(struct dcp_profile *prof, FILE *restrict fd)
{
    struct dcp_std_profile *p = prof->vtable.derived;

    if (imm_dp_read(&p->dp.null, fd)) return DCP_RUNTIMEERROR;

    if (imm_dp_read(&p->dp.alt, fd)) return DCP_RUNTIMEERROR;

    return DCP_SUCCESS;
}

static enum dcp_rc write(struct dcp_profile const *prof, FILE *restrict fd)
{
    struct dcp_std_profile const *p = prof->vtable.derived;

    if (imm_dp_write(&p->dp.null, fd)) return DCP_RUNTIMEERROR;

    if (imm_dp_write(&p->dp.alt, fd)) return DCP_RUNTIMEERROR;

    return DCP_SUCCESS;
}

static void del(struct dcp_profile *prof)
{
    if (prof)
    {
        struct dcp_std_profile *p = prof->vtable.derived;
        imm_del(&p->dp.null);
        imm_del(&p->dp.alt);
    }
}
