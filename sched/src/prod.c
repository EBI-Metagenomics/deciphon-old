#include "prod.h"
#include "common/compiler.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/safe.h"
#include "common/to.h"
#include "common/xfile.h"
#include "sched/prod.h"
#include "sched/sched.h"
#include "stmt.h"
#include "tok.h"
#include "xsql.h"
#include <assert.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdlib.h>

enum
{
    COL_TYPE_INT,
    COL_TYPE_INT64,
    COL_TYPE_DOUBLE,
    COL_TYPE_TEXT
} col_type[9] = {COL_TYPE_INT64, COL_TYPE_INT64,  COL_TYPE_TEXT,
                 COL_TYPE_TEXT,  COL_TYPE_DOUBLE, COL_TYPE_DOUBLE,
                 COL_TYPE_TEXT,  COL_TYPE_TEXT,   COL_TYPE_TEXT};

extern struct sqlite3 *sched;
static TOK_DECLARE(tok);
static unsigned nthreads = 0;
static struct xfile_tmp prod_file[MAX_NUM_THREADS] = {0};

void sched_prod_init(struct sched_prod *prod, int64_t job_id)
{
    prod->id = 0;

    prod->job_id = job_id;
    prod->seq_id = 0;

    prod->profile_name[0] = 0;
    prod->abc_name[0] = 0;

    prod->alt_loglik = 0.;
    prod->null_loglik = 0.;

    prod->profile_typeid[0] = 0;
    prod->version[0] = 0;

    prod->match[0] = 0;
}

static void cleanup(void)
{
    for (unsigned i = 0; i < nthreads; ++i)
        xfile_tmp_del(prod_file + i);
    nthreads = 0;
}

enum rc prod_begin_submission(unsigned num_threads)
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
    return RC_DONE;
}

enum rc sched_prod_write_begin(struct sched_prod const *prod,
                               unsigned thread_num)
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

    return RC_DONE;

#undef Fg
#undef Fs
#undef Fd64
#undef echo
#undef TAB
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

enum rc sched_prod_write_match(sched_prod_write_match_cb *cb, void const *match,
                               unsigned thread_num)
{
    return cb(prod_file[thread_num].fp, match);
}

enum rc sched_prod_write_match_sep(unsigned thread_num)
{
    if (fputc(';', prod_file[thread_num].fp) == EOF) return eio("fputc");
    return RC_DONE;
}

enum rc sched_prod_write_end(unsigned thread_num)
{
    if (fputc('\n', prod_file[thread_num].fp) == EOF) return eio("fputc");
    return RC_DONE;
}

static enum rc submit_prod_file(FILE *restrict fp);

enum rc prod_end_submission(void)
{
    int rc = RC_EFAIL;

    for (unsigned i = 0; i < nthreads; ++i)
    {
        if (fflush(prod_file[i].fp))
        {
            rc = eio("fflush");
            goto cleanup;
        }
        rewind(prod_file[i].fp);
        if ((rc = submit_prod_file(prod_file[i].fp))) goto cleanup;
    }
    rc = RC_DONE;

cleanup:
    cleanup();
    return rc;
}

static enum rc get_prod(struct sched_prod *prod)
{
    struct sqlite3_stmt *st = stmt[PROD_SELECT];
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, prod->id)) return efail("bind");

    if (xsql_step(st) != RC_NEXT) return efail("step");

    int i = 0;
    prod->id = sqlite3_column_int64(st, i++);

    prod->job_id = sqlite3_column_int64(st, i++);
    prod->seq_id = sqlite3_column_int64(st, i++);

#define ecpy efail("copy txt")

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, profile_name))) return ecpy;
    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, abc_name))) return ecpy;

    prod->alt_loglik = sqlite3_column_double(st, i++);
    prod->null_loglik = sqlite3_column_double(st, i++);

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, profile_typeid))) return ecpy;
    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, version))) return ecpy;

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, match))) return ecpy;

    if (xsql_step(st)) return efail("step");
    return RC_DONE;

#undef ecpy
}

enum rc sched_prod_next(struct sched_prod *prod)
{
    struct sqlite3_stmt *st = stmt[PROD_SELECT_NEXT];
    int rc = RC_DONE;
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, prod->id)) return efail("bind");
    if (xsql_bind_i64(st, 1, prod->job_id)) return efail("bind");

    rc = xsql_step(st);
    if (rc == RC_DONE) return RC_DONE;
    if (rc != RC_NEXT) return efail("step");

    prod->id = sqlite3_column_int64(st, 0);
    if (xsql_step(st)) return efail("step");

    if (get_prod(prod)) return RC_EFAIL;
    return RC_NEXT;
}

#define CLEANUP()                                                              \
    do                                                                         \
    {                                                                          \
        efail("submit prod");                                                  \
        goto cleanup;                                                          \
    } while (1)

static enum rc submit_prod_file(FILE *restrict fp)
{
    if (xsql_begin_transaction(sched)) CLEANUP();

    struct sqlite3_stmt *st = stmt[PROD_INSERT];

    do
    {
        if (xsql_reset(st)) CLEANUP();
        if (tok_next(&tok, fp)) CLEANUP();
        if (tok_id(&tok) == TOK_EOF) break;

        for (int i = 0; i < (int)ARRAY_SIZE(col_type); i++)
        {
            if (col_type[i] == COL_TYPE_INT64)
            {
                int64_t val = 0;
                if (!to_int64(tok_value(&tok), &val)) CLEANUP();
                if (xsql_bind_i64(st, i, val)) CLEANUP();
            }
            else if (col_type[i] == COL_TYPE_DOUBLE)
            {
                double val = 0;
                if (!to_double(tok_value(&tok), &val)) CLEANUP();
                if (xsql_bind_dbl(st, i, val)) CLEANUP();
            }
            else if (col_type[i] == COL_TYPE_TEXT)
            {
                struct xsql_txt txt = {tok_size(&tok), tok_value(&tok)};
                if (xsql_bind_txt(st, i, txt)) CLEANUP();
            }
            if (tok_next(&tok, fp)) CLEANUP();
        }
        assert(tok_id(&tok) == TOK_NL);
        if (xsql_step(st) != RC_DONE) CLEANUP();
    } while (true);

    if (xsql_end_transaction(sched)) CLEANUP();
    return RC_DONE;

cleanup:
    xsql_rollback_transaction(sched);
    return RC_EFAIL;
}

#undef CLEANUP
