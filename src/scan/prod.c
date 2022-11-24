#include "scan/prod.h"
#include "core/version.h"
#include "fs.h"
#include "imm/imm.h"
#include "logy.h"
#include "repr_size.h"
#include "scan/match.h"
#include <string.h>

void prod_setup_job(struct prod *prod, char const *abc_name,
                    char const *prof_typeid, long scan_id)
{
    strcpy(prod->abc_name, abc_name);
    strcpy(prod->profile_typeid, prof_typeid);
    strcpy(prod->version, DECIPHON_VERSION);
    prod->scan_id = scan_id;
}

void prod_setup_seq(struct prod *prod, long seq_id) { prod->seq_id = seq_id; }

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

static int write_begin(struct prod const *prod, FILE *fp);
static int write_match_sep(FILE *fp);
static int write_end(FILE *fp);

int prod_write(struct prod const *prod, struct imm_seq const *seq,
               struct imm_path const *path,
               prod_fwrite_match_fn_t *fwrite_match, struct match *match,
               FILE *fp)
{
    enum rc rc = RC_OK;

    if (write_begin(prod, fp)) return eio("failed to write prod");

    unsigned start = 0;
    for (unsigned idx = 0; idx < imm_path_nsteps(path); idx++)
    {
        match->step = imm_path_step(path, idx);
        struct imm_seq frag = imm_subseq(seq, start, match->step->seqlen);
        match->frag = &frag;

        if (idx > 0 && idx + 1 <= imm_path_nsteps(path))
        {
            if (write_match_sep(fp)) return eio("failed to write prod");
        }

        if ((*fwrite_match)(fp, match)) return eio("write prod");

        start += match->step->seqlen;
    }
    if (write_end(fp)) return eio("failed to write prod");

    return rc;
}

static int write_begin(struct prod const *prod, FILE *fp)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(fp, fmt, prod->var) < 0
#define Fi "%ld" TAB
#define Fs "%s" TAB
#define Fg "%" repr_size_dbl_fmt TAB

    if (echo(Fi, scan_id)) efail("write prod");
    if (echo(Fi, seq_id)) efail("write prod");

    if (echo(Fs, profile_name)) efail("write prod");
    if (echo(Fs, abc_name)) efail("write prod");

    if (echo(Fg, alt_loglik)) efail("write prod");
    if (echo(Fg, null_loglik)) efail("write prod");

    if (echo(Fs, profile_typeid)) efail("write prod");
    if (echo(Fs, version)) efail("write prod");

    return RC_OK;

#undef Fg
#undef Fs
#undef Fi
#undef echo
#undef TAB
}

static int write_match_sep(FILE *fp)
{
    return fputc(';', fp) == EOF ? eio("fputc") : RC_OK;
}

static int write_end(FILE *fp)
{
    return fputc('\n', fp) == EOF ? eio("fputc") : RC_OK;
}
