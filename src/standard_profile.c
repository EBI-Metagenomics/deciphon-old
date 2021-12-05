#include "standard_profile.h"
#include "imm/imm.h"
#include "meta.h"
#include "profile.h"
#include "rc.h"
#include "standard_profile.h"

static void del(struct profile *prof);

void standard_profile_init(struct standard_profile *prof,
                           struct imm_code const *code)
{
    imm_dp_init(&prof->dp.null, code);
    imm_dp_init(&prof->dp.alt, code);
    struct profile_vtable vtable = {del, STANDARD_PROFILE};
    profile_init(&prof->super, code, meta_unset, vtable);
}

enum rc standard_profile_read(struct standard_profile *prof, FILE *restrict fd)
{
    if (imm_dp_read(&prof->dp.null, fd)) return RC_FAIL;
    if (imm_dp_read(&prof->dp.alt, fd)) return RC_FAIL;
    return RC_DONE;
}

enum rc standard_profile_write(struct standard_profile const *prof,
                               FILE *restrict fd)
{
    if (imm_dp_write(&prof->dp.null, fd)) return RC_FAIL;
    if (imm_dp_write(&prof->dp.alt, fd)) return RC_FAIL;
    return RC_DONE;
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
