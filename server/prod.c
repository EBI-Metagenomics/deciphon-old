#include "prod.h"
#include "deciphon/logger.h"
#include "deciphon/model/profile.h"
#include "deciphon/server/rest.h"
#include "deciphon/xfile.h"
#include "imm/imm.h"
#include "match.h"
#include <inttypes.h>

static unsigned nthreads = 0;
static struct xfile_tmp prod_file[MAX_NUM_THREADS] = {0};

static void cleanup(void)
{
    for (unsigned i = 0; i < nthreads; ++i)
        xfile_tmp_del(prod_file + i);
    nthreads = 0;
}

static enum rc write_begin(struct prod const *prod, unsigned thread_num)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(prod_file[thread_num].fp, fmt, prod->var) < 0
#define Fd64 "%" PRId64 TAB
#define Fs "%s" TAB
#define Fg "%.17g" TAB

    if (echo(Fd64, job_id)) efail("write prod");
    if (echo(Fd64, seq_id)) efail("write prod");

    if (echo(Fs, profile_name)) efail("write prod");
    if (echo(Fs, abc_name)) efail("write prod");

    /* Reference: https://stackoverflow.com/a/21162120 */
    if (echo(Fg, alt_loglik)) efail("write prod");
    if (echo(Fg, null_loglik)) efail("write prod");

    if (echo(Fs, profile_typeid)) efail("write prod");
    if (echo(Fs, version)) efail("write prod");

    return RC_OK;

#undef Fg
#undef Fs
#undef Fd64
#undef echo
#undef TAB
}

static enum rc write_match_sep(unsigned thread_num)
{
    if (fputc(';', prod_file[thread_num].fp) == EOF) return eio("fputc");
    return RC_OK;
}

static enum rc write_end(unsigned thread_num)
{
    if (fputc('\n', prod_file[thread_num].fp) == EOF) return eio("fputc");
    return RC_OK;
}

enum rc prod_fopen(unsigned num_threads)
{
    assert(num_threads <= MAX_NUM_THREADS);
    for (nthreads = 0; nthreads < num_threads; ++nthreads)
    {
        if (xfile_tmp_open(prod_file + nthreads))
        {
            cleanup();
            return efail("begin prod submission");
        }
    }
    return RC_OK;
}

/* Output example
 *             ___________________________
 *             |   match0   |   match1   |
 *             ---------------------------
 * Output----->| CG,M1,CGA,K;CG,M4,CGA,K |
 *             ---|-|---|--|--------------
 * -----------   /  |   |  \    ---------------
 * | matched |__/   |   |   \___| most likely |
 * | letters |      |   |       | amino acid  |
 * -----------      |   |       ---------------
 *      -------------   ---------------
 *      | hmm state |   | most likely |
 *      -------------   | codon       |
 *                      ---------------
 */

enum rc prod_fclose(void)
{
    enum rc rc = RC_EFAIL;

    for (unsigned i = 0; i < nthreads; ++i)
    {
        if (fflush(prod_file[i].fp))
        {
            rc = eio("fflush");
            goto cleanup;
        }
        rewind(prod_file[i].fp);
#if 0
        if ((rc = rest_submit_prods_file(prod_file[i].path))) goto cleanup;
#endif
    }
    rc = RC_OK;

cleanup:
    cleanup();
    return rc;
}

enum rc prod_fwrite(struct prod const *prod, struct imm_seq const *seq,
                    struct imm_path const *path, unsigned partition,
                    prod_fwrite_match_cb fwrite_match, struct match *match)
{
    enum rc rc = RC_OK;

    if (write_begin(prod, partition))
        return error(RC_EIO, "failed to write prod");

    unsigned start = 0;
    for (unsigned idx = 0; idx < imm_path_nsteps(path); idx++)
    {
        match->step = imm_path_step(path, idx);
        struct imm_seq frag = imm_subseq(seq, start, match->step->seqlen);
        match->frag = &frag;

        if (idx > 0 && idx + 1 <= imm_path_nsteps(path))
        {
            if (write_match_sep(partition))
                return error(RC_EIO, "failed to write prod");
        }

        if (fwrite_match(prod_file[partition].fp, match))
            return eio("write prod");

        start += match->step->seqlen;
    }
    if (write_end(partition)) return error(RC_EIO, "failed to write prod");

    return rc;
}
