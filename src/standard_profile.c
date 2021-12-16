#include "standard_profile.h"
#include "cmp/cmp.h"
#include "imm/imm.h"
#include "metadata.h"
#include "profile.h"
#include "profile_types.h"
#include "rc.h"
#include "standard_profile.h"
#include "standard_state.h"

static void del(struct profile *prof)
{
    if (prof)
    {
        struct standard_profile *p = (struct standard_profile *)prof;
        imm_del(&p->dp.null);
        imm_del(&p->dp.alt);
    }
}

static enum rc read(struct profile *prof, struct cmp_ctx_s *cmp)
{
    struct standard_profile *p = (struct standard_profile *)prof;
    FILE *fp = cmp_file(cmp);
    if (imm_dp_read(&p->dp.null, fp)) return RC_FAIL;
    if (imm_dp_read(&p->dp.alt, fp)) return RC_FAIL;
    return RC_DONE;
}

static struct imm_dp const *null_dp(struct profile const *prof)
{
    struct standard_profile *p = (struct standard_profile *)prof;
    return &p->dp.null;
}

static struct imm_dp const *alt_dp(struct profile const *prof)
{
    struct standard_profile *p = (struct standard_profile *)prof;
    return &p->dp.alt;
}

static struct profile_vtable vtable = {PROFILE_STANDARD, del, read, null_dp,
                                       alt_dp};

void standard_profile_init(struct standard_profile *prof,
                           struct imm_code const *code)
{
    imm_dp_init(&prof->dp.null, code);
    imm_dp_init(&prof->dp.alt, code);
    profile_init(&prof->super, code, metadata_unset(), vtable,
                 standard_state_name);
}

enum rc standard_profile_write(struct standard_profile const *prof,
                               FILE *restrict fd)
{
    if (imm_dp_write(&prof->dp.null, fd)) return RC_FAIL;
    if (imm_dp_write(&prof->dp.alt, fd)) return RC_FAIL;
    return RC_DONE;
}
