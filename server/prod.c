#include "prod.h"
#include "deciphon/logger.h"
#include "deciphon/version.h"
#include "deciphon/xfile.h"
#include "imm/imm.h"
#include "match.h"
#include <inttypes.h>

static unsigned num_threads = 0;
static struct xfile_tmp prod_file[NUM_THREADS] = {0};
static struct xfile_tmp final_file = {0};

static enum rc write_begin(struct prod const *prod, unsigned thread_num)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(prod_file[thread_num].fp, fmt, prod->var) < 0
#define Fd64 "%" PRId64 TAB
#define Fs "%s" TAB
#define Fg "%.17g" TAB

    if (echo(Fd64, scan_id)) efail("write prod");
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
    FILE *fp = prod_file[thread_num].fp;
    return fputc(';', fp) == EOF ? eio("fputc") : RC_OK;
}

static enum rc write_end(unsigned thread_num)
{
    FILE *fp = prod_file[thread_num].fp;
    return fputc('\n', fp) == EOF ? eio("fputc") : RC_OK;
}

enum rc prod_fopen(unsigned nthreads)
{
    assert(nthreads <= NUM_THREADS);
    for (num_threads = 0; num_threads < nthreads; ++num_threads)
    {
        if (xfile_tmp_open(prod_file + num_threads))
        {
            prod_fcleanup();
            return efail("begin prod submission");
        }
    }
    return RC_OK;
}

void prod_setup_job(struct prod *prod, char const *abc_name,
                    char const *prof_typeid, int64_t scan_id)
{
    strcpy(prod->abc_name, abc_name);
    strcpy(prod->profile_typeid, prof_typeid);
    strcpy(prod->version, DECIPHON_VERSION);
    prod->scan_id = scan_id;
}

void prod_setup_seq(struct prod *prod, int64_t seq_id)
{
    prod->seq_id = seq_id;
}

void prod_fcleanup(void)
{
    for (unsigned i = 0; i < num_threads; ++i)
        xfile_tmp_del(prod_file + i);
    num_threads = 0;
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
    enum rc rc = RC_OK;

    if (xfile_tmp_open(&final_file))
    {
        rc = eio("fail to finish product");
        goto cleanup;
    }
    fputs("scan_id	seq_id	profile_name	abc_name	alt_loglik	"
          "null_loglik	profile_typeid	version	match\n",
          final_file.fp);

    for (unsigned i = 0; i < num_threads; ++i)
    {
        if (fflush(prod_file[i].fp))
        {
            rc = eio("failed to flush");
            xfile_tmp_del(&final_file);
            goto cleanup;
        }
        rewind(prod_file[i].fp);
        rc = xfile_copy(final_file.fp, prod_file[i].fp);
        if (rc)
        {
            xfile_tmp_del(&final_file);
            goto cleanup;
        }
    }
    if (fflush(final_file.fp))
    {
        rc = eio("failed to flush");
        goto cleanup;
    }
    rewind(final_file.fp);

cleanup:
    prod_fcleanup();
    return rc;
}

FILE *prod_final_fp(void) { return final_file.fp; }

char const *prod_final_path(void) { return final_file.path; }

void prod_final_cleanup(void) { xfile_tmp_del(&final_file); }

enum rc prod_fwrite(struct prod const *prod, struct imm_seq const *seq,
                    struct imm_path const *path, unsigned thread_num,
                    prod_fwrite_match_func_t fwrite_match, struct match *match)
{
    enum rc rc = RC_OK;

    if (write_begin(prod, thread_num)) return eio("failed to write prod");

    unsigned start = 0;
    for (unsigned idx = 0; idx < imm_path_nsteps(path); idx++)
    {
        match->step = imm_path_step(path, idx);
        struct imm_seq frag = imm_subseq(seq, start, match->step->seqlen);
        match->frag = &frag;

        if (idx > 0 && idx + 1 <= imm_path_nsteps(path))
        {
            if (write_match_sep(thread_num)) return eio("failed to write prod");
        }

        if (fwrite_match(prod_file[thread_num].fp, match))
            return eio("write prod");

        start += match->step->seqlen;
    }
    if (write_end(thread_num)) return eio("failed to write prod");

    return rc;
}
