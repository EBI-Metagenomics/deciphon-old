#include "model/standard_profile.h"
#include "imm/imm.h"
#include "lite_pack/file/file.h"
#include "lite_pack/lite_pack.h"
#include "logy.h"
#include "model/profile.h"
#include "model/profile_typeid.h"
#include "model/standard_profile.h"
#include "model/standard_state.h"
#include "rc.h"

static void del(struct profile *prof)
{
    if (prof)
    {
        struct standard_profile *p = (struct standard_profile *)prof;
        imm_del(&p->dp.null);
        imm_del(&p->dp.alt);
    }
}

static enum rc read(struct profile *prof, struct lip_file *file)
{
    struct standard_profile *p = (struct standard_profile *)prof;
    if (imm_dp_unpack(&p->dp.null, file)) return RC_EFAIL;
    if (imm_dp_unpack(&p->dp.alt, file)) return RC_EFAIL;
    return RC_OK;
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

void standard_profile_init(struct standard_profile *prof, char const *accession,
                           struct imm_code const *code)
{
    imm_dp_init(&prof->dp.null, code);
    imm_dp_init(&prof->dp.alt, code);
    profile_init(&prof->super, accession, code, vtable, standard_state_name);
}

enum rc standard_profile_pack(struct standard_profile const *prof,
                              struct lip_file *file)
{
    if (!lip_write_cstr(file, "null")) return eio("write null state key");
    if (imm_dp_pack(&prof->dp.null, file)) return RC_EFAIL;

    if (!lip_write_cstr(file, "alt")) return eio("write alt state key");
    if (imm_dp_pack(&prof->dp.alt, file)) return RC_EFAIL;

    return RC_OK;
}
