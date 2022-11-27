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

int prod_write_begin(struct prod const *prod, FILE *fp)
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
    if (echo(Fg, evalue_log)) efail("write prod");

    if (echo(Fs, profile_typeid)) efail("write prod");
    if (echo(Fs, version)) efail("write prod");

    return RC_OK;

#undef Fg
#undef Fs
#undef Fi
#undef echo
#undef TAB
}

int prod_write_sep(FILE *fp)
{
    return fputc(';', fp) == EOF ? eio("fputc") : RC_OK;
}

int prod_write_end(FILE *fp)
{
    return fputc('\n', fp) == EOF ? eio("fputc") : RC_OK;
}
