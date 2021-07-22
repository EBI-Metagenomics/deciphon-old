#include "dcp/std_profile.h"
#include "imm/imm.h"
#include "profile.h"
#include "support.h"

static int read(struct dcp_profile *prof, FILE *restrict fd);

static int write(struct dcp_profile const *prof, FILE *restrict fd);

static void del(struct dcp_profile const *prof);

DCP_API struct dcp_std_profile *dcp_std_profile_new(struct imm_abc const *abc,
                                                    struct dcp_meta mt)
{
    struct dcp_std_profile *prof = xmalloc(sizeof(*prof));
    prof->dp.null = imm_dp_new(abc);
    prof->dp.alt = imm_dp_new(abc);
    struct dcp_profile_vtable vtable = {read, write, del, DCP_STD_PROFILE,
                                        prof};
    prof->super = profile_new(abc, mt, vtable);
    return prof;
}

void dcp_std_profile_reset(struct dcp_std_profile *p, struct dcp_meta mt)
{
    p->super->mt = mt;
}

static int read(struct dcp_profile *prof, FILE *restrict fd)
{
    int rc = IMM_SUCCESS;
    struct dcp_std_profile const *p = prof->vtable.derived;

    if ((rc = imm_dp_read(p->dp.null, fd)))
        return rc;

    if ((rc = imm_dp_read(p->dp.alt, fd)))
        return rc;

    return rc;
}

static int write(struct dcp_profile const *prof, FILE *restrict fd)
{
    int rc = IMM_SUCCESS;
    struct dcp_std_profile const *p = prof->vtable.derived;

    if ((rc = imm_dp_write(p->dp.null, fd)))
        return rc;

    if ((rc = imm_dp_write(p->dp.alt, fd)))
        return rc;

    return rc;
}

static void del(struct dcp_profile const *prof)
{
    if (prof)
    {
        struct dcp_std_profile const *p = prof->vtable.derived;
        imm_dp_del(p->dp.null);
        imm_dp_del(p->dp.alt);
        free((void *)p);
        profile_del(prof);
    }
}
