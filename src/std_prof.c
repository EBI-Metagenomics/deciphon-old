#include "std_prof.h"
#include "imm/imm.h"
#include "meta.h"
#include "prof.h"
#include "rc.h"
#include "std_prof.h"

static void del(struct profile *prof);

void standard_profile_init(struct standard_profile *prof, struct imm_code const *code)
{
    imm_dp_init(&prof->dp.null, code);
    imm_dp_init(&prof->dp.alt, code);
    struct dcp_prof_vtable vtable = {del, DCP_STD_PROFILE};
    profile_init(&prof->super, code, meta_unset, vtable);
}

enum rc standard_profile_read(struct standard_profile *prof, FILE *restrict fd)
{
    if (imm_dp_read(&prof->dp.null, fd)) return FAIL;
    if (imm_dp_read(&prof->dp.alt, fd)) return FAIL;
    return DONE;
}

enum rc standard_profile_write(struct standard_profile const *prof, FILE *restrict fd)
{
    if (imm_dp_write(&prof->dp.null, fd)) return FAIL;
    if (imm_dp_write(&prof->dp.alt, fd)) return FAIL;
    return DONE;
}

static void del(struct profile *prof)
{
    if (prof)
    {
        struct standard_profile *p = (struct standard_profile *)prof;
        imm_del(&p->dp.null);
        imm_del(&p->dp.alt);
    }
}
