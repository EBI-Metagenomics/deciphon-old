#include "dcp/std_prof.h"
#include "dcp/rc.h"
#include "imm/imm.h"
#include "prof.h"
#include "std_prof.h"

static void del(struct dcp_prof *prof);

void dcp_std_prof_init(struct dcp_std_prof *prof, struct imm_abc const *abc)
{
    imm_dp_init(&prof->dp.null, abc);
    imm_dp_init(&prof->dp.alt, abc);
    struct dcp_prof_vtable vtable = {del, DCP_STD_PROFILE};
    profile_init(&prof->super, abc, dcp_meta(NULL, NULL), vtable);
}

enum dcp_rc std_prof_read(struct dcp_std_prof *prof, FILE *restrict fd)
{
    if (imm_dp_read(&prof->dp.null, fd)) return DCP_RUNTIMEERROR;
    if (imm_dp_read(&prof->dp.alt, fd)) return DCP_RUNTIMEERROR;
    return DCP_SUCCESS;
}

enum dcp_rc std_prof_write(struct dcp_std_prof const *prof, FILE *restrict fd)
{
    if (imm_dp_write(&prof->dp.null, fd)) return DCP_RUNTIMEERROR;
    if (imm_dp_write(&prof->dp.alt, fd)) return DCP_RUNTIMEERROR;
    return DCP_SUCCESS;
}

static void del(struct dcp_prof *prof)
{
    if (prof)
    {
        struct dcp_std_prof *p = (struct dcp_std_prof *)prof;
        imm_del(&p->dp.null);
        imm_del(&p->dp.alt);
    }
}
