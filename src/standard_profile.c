#include "standard_profile.h"
#include "common/logger.h"
#include "common/rc.h"
#include "imm/imm.h"
#include "js.h"
#include "metadata.h"
#include "profile.h"
#include "profile_types.h"
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

static enum rc read(struct profile *prof, struct lip_file *cmp)
{
    struct standard_profile *p = (struct standard_profile *)prof;
    if (imm_dp_read_cmp(&p->dp.null, cmp)) return RC_EFAIL;
    if (imm_dp_read_cmp(&p->dp.alt, cmp)) return RC_EFAIL;
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
    profile_init(&prof->super, code, vtable, standard_state_name);
}

enum rc standard_profile_write(struct standard_profile const *prof,
                               struct lip_file *cmp)
{
    FILE *fp = cmp_file(cmp);

    if (!JS_WRITE_STR(cmp, "null")) return eio("write null state key");
    if (imm_dp_write(&prof->dp.null, fp)) return RC_EFAIL;

    if (!JS_WRITE_STR(cmp, "alt")) return eio("write alt state key");
    if (imm_dp_write(&prof->dp.alt, fp)) return RC_EFAIL;

    return RC_DONE;
}
