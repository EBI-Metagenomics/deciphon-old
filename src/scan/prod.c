#include "scan/prod.h"
#include "core/limits.h"
#include "core/logging.h"
#include "core/version.h"
#include "imm/imm.h"
#include "scan/match.h"
#include "xfile.h"
#include <inttypes.h>

static unsigned num_threads = 0;
static FILE *prod_file[NUM_THREADS] = {NULL};
static struct
{
    FILE *file;
    char path[PATH_SIZE];
} final = {0};

static enum rc write_begin(struct prod const *prod, unsigned thread_num)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(prod_file[thread_num], fmt, prod->var) < 0
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
    FILE *fp = prod_file[thread_num];
    return fputc(';', fp) == EOF ? eio("fputc") : RC_OK;
}

static enum rc write_end(unsigned thread_num)
{
    FILE *fp = prod_file[thread_num];
    return fputc('\n', fp) == EOF ? eio("fputc") : RC_OK;
}

enum rc prod_fopen(unsigned nthreads)
{
    assert(nthreads <= NUM_THREADS);
    for (num_threads = 0; num_threads < nthreads; ++num_threads)
    {
        if (!(prod_file[num_threads] = tmpfile()))
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
        fclose(prod_file[i]);
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

static enum rc prod_final_fopen(void);
static enum rc prod_final_fclose(bool succesfully);

enum rc prod_fclose(void)
{
    enum rc rc = prod_final_fopen();
    if (rc) goto cleanup;

    fputs("scan_id	seq_id	profile_name	abc_name	alt_loglik	"
          "null_loglik	profile_typeid	version	match\n",
          final.file);

    for (unsigned i = 0; i < num_threads; ++i)
    {
        if (fflush(prod_file[i]))
        {
            rc = eio("failed to flush");
            goto cleanup;
        }
        rewind(prod_file[i]);
        int r = xfile_copy(final.file, prod_file[i]);
        if (r)
        {
            rc = eio(xfile_strerror(r));
            goto cleanup;
        }
    }
    if (fflush(final.file))
    {
        rc = eio("failed to flush");
        goto cleanup;
    }
    rewind(final.file);

cleanup:
    prod_fcleanup();
    enum rc rc_pf = prod_final_fclose(!rc);
    return rc ? rc : rc_pf;
}

char const *prod_final_path(void) { return final.path; }

enum rc prod_fwrite(struct prod const *prod, struct imm_seq const *seq,
                    struct imm_path const *path, unsigned thread_num,
                    prod_fwrite_match_fn_t *fwrite_match, struct match *match)
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

        if (fwrite_match(prod_file[thread_num], match))
            return eio("write prod");

        start += match->step->seqlen;
    }
    if (write_end(thread_num)) return eio("failed to write prod");

    return rc;
}

static enum rc prod_final_fopen(void)
{
    final.file = NULL;
    if (xfile_mkstemp(PATH_SIZE, final.path))
        return eio("fail to finish product");

    final.file = fopen(final.path, "wb");
    return final.file ? RC_OK : eio("fail to finish product");
}

static enum rc prod_final_fclose(bool succesfully)
{
    FILE *fp = final.file;
    final.file = NULL;

    if (succesfully)
        return fclose(fp) ? eio("failed to close final product") : RC_OK;

    return RC_OK;
}
