#include "std_prof.h"
#include "imm/imm.h"
#include "meta.h"
#include "prof.h"
#include "rc.h"
#include "std_prof.h"

static void del(struct dcp_prof *prof);

void dcp_std_prof_init(struct dcp_std_prof *prof, struct imm_code const *code)
{
    imm_dp_init(&prof->dp.null, code);
    imm_dp_init(&prof->dp.alt, code);
    struct dcp_prof_vtable vtable = {del, DCP_STD_PROFILE};
    profile_init(&prof->super, code, meta_unset, vtable);
}

enum rc std_prof_read(struct dcp_std_prof *prof, FILE *restrict fd)
{
    if (imm_dp_read(&prof->dp.null, fd)) return FAIL;
    if (imm_dp_read(&prof->dp.alt, fd)) return FAIL;
    return DONE;
}

enum rc std_prof_write(struct dcp_std_prof const *prof, FILE *restrict fd)
{
    if (imm_dp_write(&prof->dp.null, fd)) return FAIL;
    if (imm_dp_write(&prof->dp.alt, fd)) return FAIL;
    return DONE;
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
