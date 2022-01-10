#include "prod.h"
#include "dcp_sched/sched.h"
#include "imm/imm.h"
#include "common/logger.h"
#include "match.h"
#include "profile.h"

enum rc prod_write(struct sched_prod const *prod, struct imm_seq const *seq,
                   struct imm_path const *path, unsigned partition,
                   sched_prod_write_match_cb write_match_cb,
                   struct match *match)
{
    enum rc rc = RC_DONE;

    if (sched_prod_write_begin(prod, partition))
        return error(RC_IOERROR, "failed to write prod");

    unsigned start = 0;
    for (unsigned idx = 0; idx < imm_path_nsteps(path); idx++)
    {
        match->step = imm_path_step(path, idx);
        struct imm_seq frag = imm_subseq(seq, start, match->step->seqlen);
        match->frag = &frag;

        if (idx > 0 && idx + 1 <= imm_path_nsteps(path))
        {
            if (sched_prod_write_match_sep(partition))
                return error(RC_IOERROR, "failed to write prod");
        }

        if (sched_prod_write_match(write_match_cb, match, partition))
            return error(RC_IOERROR, "failed to write prod");

        start += match->step->seqlen;
    }
    if (sched_prod_write_end(partition))
        return error(RC_IOERROR, "failed to write prod");

    return rc;
}
