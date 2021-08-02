#include "dcp/std_profile.h"
#include "imm/imm.h"
#include "profile.h"
#include "support.h"

static int read(struct dcp_profile *prof, FILE *restrict fd);

static int write(struct dcp_profile const *prof, FILE *restrict fd);

static void del(struct dcp_profile *prof);

DCP_API void dcp_std_profile_init(struct dcp_std_profile *p,
                                  struct imm_abc const *abc)
{
    imm_dp_init(&p->dp.null, abc);
    imm_dp_init(&p->dp.alt, abc);
    struct dcp_profile_vtable vtable = {read, write, del, DCP_STD_PROFILE, p};
    profile_init(&p->super, abc, dcp_meta(NULL, NULL), vtable);
}

static int read(struct dcp_profile *prof, FILE *restrict fd)
{
    int rc = IMM_SUCCESS;
    struct dcp_std_profile *p = prof->vtable.derived;

    if ((rc = imm_dp_read(&p->dp.null, fd)))
        return rc;

    if ((rc = imm_dp_read(&p->dp.alt, fd)))
        return rc;

    return rc;
}

static int write(struct dcp_profile const *prof, FILE *restrict fd)
{
    int rc = IMM_SUCCESS;
    struct dcp_std_profile const *p = prof->vtable.derived;

    if ((rc = imm_dp_write(&p->dp.null, fd)))
        return rc;

    if ((rc = imm_dp_write(&p->dp.alt, fd)))
        return rc;

    return rc;
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
