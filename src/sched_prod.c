#include "sched_prod.h"
#include "array.h"
#include "compiler.h"
#include "logger.h"
#include "protein_match.h"
#include "safe.h"
#include "sched.h"
#include "to.h"
#include "tok.h"
#include "xsql.h"
#include <inttypes.h>
#include <sqlite3.h>
#include <stdlib.h>

enum
{
    INSERT,
    SELECT,
    SELECT_NEXT
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO prod\
            (\
                job_id,           seq_id,   match_id,\
                prof_name,      abc_name,            \
                loglik,      null_loglik,            \
                prof_typeid,     version,            \
                match_data                           \
            )\
        VALUES\
            (\
                ?, ?, ?, \
                ?, ?,\
                ?, ?,\
                ?, ?,\
                ?\
            ) RETURNING id;\
",
    [SELECT] = "SELECT * FROM prod WHERE id = ?;\
",
    [SELECT_NEXT] = \
"\
        SELECT\
            id FROM prod\
        WHERE\
            id > ? AND job_id = ? ORDER BY id ASC LIMIT 1;\
"};
/* clang-format on */

enum
{
    COL_TYPE_INT,
    COL_TYPE_INT64,
    COL_TYPE_DOUBLE,
    COL_TYPE_TEXT
} col_type[12] = {COL_TYPE_INT64,  COL_TYPE_INT64, COL_TYPE_INT64,
                  COL_TYPE_TEXT,   COL_TYPE_TEXT,  COL_TYPE_DOUBLE,
                  COL_TYPE_DOUBLE, COL_TYPE_TEXT,  COL_TYPE_TEXT,
                  COL_TYPE_TEXT};

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

enum rc sched_prod_module_init(struct sqlite3 *db)
{
    enum rc rc = RC_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if ((rc = xsql_prepare(db, queries[i], stmts + i))) return rc;
    }
    return RC_DONE;
}

enum rc sched_prod_add(struct prod *prod)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum rc rc = RC_DONE;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, prod->job_id))) return rc;
    if ((rc = xsql_bind_i64(stmt, 1, prod->seq_id))) return rc;
    if ((rc = xsql_bind_i64(stmt, 2, prod->match_id))) return rc;

    if ((rc = xsql_bind_txt(stmt, 3, XSQL_TXT_OF(*prod, prof_name)))) return rc;
    if ((rc = xsql_bind_txt(stmt, 4, XSQL_TXT_OF(*prod, abc_name)))) return rc;

    if ((rc = xsql_bind_dbl(stmt, 5, prod->loglik))) return rc;
    if ((rc = xsql_bind_dbl(stmt, 6, prod->null_loglik))) return rc;

    if ((rc = xsql_bind_txt(stmt, 7, XSQL_TXT_OF(*prod, prof_typeid))))
        return rc;
    if ((rc = xsql_bind_txt(stmt, 8, XSQL_TXT_OF(*prod, version)))) return rc;

    struct xsql_txt match = {(unsigned)array_size(prod->match),
                             array_data(prod->match)};
    if ((rc = xsql_bind_txt(stmt, 9, match))) return rc;

    rc = xsql_step(stmt);
    if (rc != RC_NEXT) return rc;
    prod->id = sqlite3_column_int64(stmt, 0);
    return xsql_end_step(stmt);
}

enum rc sched_prod_next(int64_t job_id, int64_t *prod_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT_NEXT];
    enum rc rc = RC_DONE;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, *prod_id))) return rc;
    if ((rc = xsql_bind_i64(stmt, 1, job_id))) return rc;

    rc = xsql_step(stmt);
    if (rc == RC_DONE) return rc;
    if (rc != RC_NEXT) return rc;

    *prod_id = sqlite3_column_int64(stmt, 0);
    if ((rc = xsql_end_step(stmt))) return rc;
    return RC_NEXT;
}

enum rc sched_prod_get(struct prod *prod, int64_t prod_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum rc rc = RC_DONE;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, prod_id))) return rc;

    rc = xsql_step(stmt);
    if (rc != RC_NEXT) return error(RC_FAIL, "failed to get prod");

    prod->id = sqlite3_column_int64(stmt, 0);

    prod->job_id = sqlite3_column_int64(stmt, 1);
    prod->seq_id = sqlite3_column_int64(stmt, 2);
    prod->match_id = sqlite3_column_int64(stmt, 3);

    rc = xsql_cpy_txt(stmt, 4, XSQL_TXT_OF(*prod, prof_name));
    if (rc) return rc;
    rc = xsql_cpy_txt(stmt, 5, XSQL_TXT_OF(*prod, abc_name));
    if (rc) return rc;

    prod->loglik = sqlite3_column_double(stmt, 6);
    prod->null_loglik = sqlite3_column_double(stmt, 7);

    rc = xsql_cpy_txt(stmt, 8, XSQL_TXT_OF(*prod, prof_typeid));
    if (rc) return rc;
    rc = xsql_cpy_txt(stmt, 9, XSQL_TXT_OF(*prod, version));
    if (rc) return rc;

    struct xsql_txt txt = {0};
    if ((rc = xsql_get_txt(stmt, 10, &txt))) return rc;
    if ((rc = xsql_txt_as_array(&txt, &prod->match))) return rc;

    return xsql_end_step(stmt);
}

void sched_prod_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}

void sched_prod_set_job_id(struct prod *prod, int64_t job_id)
{
    prod->job_id = job_id;
}

void sched_prod_set_seq_id(struct prod *prod, int64_t seq_id)
{
    prod->seq_id = seq_id;
}

void sched_prod_set_match_id(struct prod *prod, int64_t match_id)
{
    prod->match_id = match_id;
}

void sched_prod_set_prof_name(struct prod *prod,
                              char const prof_name[DCP_PROF_NAME_SIZE])
{
    safe_strcpy(prod->prof_name, prof_name, DCP_PROF_NAME_SIZE);
}

void sched_prod_set_abc_name(struct prod *prod, char const *abc_name)
{
    safe_strcpy(prod->abc_name, abc_name, DCP_ABC_NAME_SIZE);
}

void sched_prod_set_loglik(struct prod *prod, double loglik)
{
    prod->loglik = loglik;
}

void sched_prod_set_null_loglik(struct prod *prod, double null_loglik)
{
    prod->null_loglik = null_loglik;
}

void sched_prod_set_prof_typeid(struct prod *prod, char const *model)
{
    safe_strcpy(prod->prof_typeid, model, DCP_PROFILE_TYPEID_SIZE);
}

void sched_prod_set_version(struct prod *prod, char const *version)
{
    safe_strcpy(prod->version, version, DCP_VERSION_SIZE);
}

enum rc sched_prod_write_preamble(struct prod *p, FILE *restrict fp)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(fp, fmt, p->var) < 0
#define Fd64 "%" PRId64 TAB
#define Fs "%s" TAB
#define Fg "%.17g" TAB

    if (echo(Fd64, job_id)) return error(RC_IOERROR, "failed to write prod");
    if (echo(Fd64, seq_id)) return error(RC_IOERROR, "failed to write prod");
    if (echo(Fd64, match_id)) return error(RC_IOERROR, "failed to write prod");

    if (echo(Fs, prof_name)) return error(RC_IOERROR, "failed to write prod");
    if (echo(Fs, abc_name)) return error(RC_IOERROR, "failed to write prod");

    /* Reference: https://stackoverflow.com/a/21162120 */
    if (echo(Fg, loglik)) return error(RC_IOERROR, "failed to write prod");
    if (echo(Fg, null_loglik)) return error(RC_IOERROR, "failed to write prod");

    if (echo(Fs, prof_typeid)) return error(RC_IOERROR, "failed to write prod");
    if (echo(Fs, version)) return error(RC_IOERROR, "failed to write prod");

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

enum rc sched_prod_write_match(FILE *restrict fd, struct protein_match const *m)
{
    if (fprintf(fd, "%.*s,", m->frag_size, m->frag) < 0)
        return error(RC_IOERROR, "failed to write frag");

    if (fprintf(fd, "%s,", m->state) < 0)
        return error(RC_IOERROR, "failed to write state");

    if (fprintf(fd, "%s,", m->codon) < 0)
        return error(RC_IOERROR, "failed to write codon");

    if (fprintf(fd, "%s", m->amino) < 0)
        return error(RC_IOERROR, "failed to write amino");

    return RC_DONE;
}

enum rc sched_prod_write_match_sep(FILE *restrict fd)
{
    if (fputc(';', fd) == EOF) return error(RC_IOERROR, "failed to write sep");
    return RC_DONE;
}

enum rc sched_prod_write_nl(FILE *restrict fd)
{
    if (fputc('\n', fd) == EOF)
        return error(RC_IOERROR, "failed to write newline");
    return RC_DONE;
}

enum rc sched_prod_add_from_tsv(FILE *restrict fd, struct tok *tok)
{
    enum rc rc = RC_DONE;
    if ((rc = xsql_begin_transaction(sched_db()))) goto cleanup;

    struct sqlite3_stmt *stmt = stmts[INSERT];

    do
    {
        if ((rc = xsql_reset(stmt))) goto cleanup;
        rc = tok_next(tok, fd);
        if (rc) return rc;
        if (tok_id(tok) == TOK_EOF) break;

        for (int i = 0; i < 10; i++)
        {
            if (col_type[i] == COL_TYPE_INT64)
            {
                int64_t val = 0;
                if (!to_int64(tok_value(tok), &val))
                {
                    error(RC_PARSEERROR, "failed to parse int64");
                    goto cleanup;
                }
                if ((rc = xsql_bind_i64(stmt, i, val))) goto cleanup;
            }
            else if (col_type[i] == COL_TYPE_DOUBLE)
            {
                double val = 0;
                if (!to_double(tok_value(tok), &val))
                {
                    error(RC_PARSEERROR, "failed to parse double");
                    goto cleanup;
                }
                if ((rc = xsql_bind_dbl(stmt, i, val))) goto cleanup;
            }
            else if (col_type[i] == COL_TYPE_TEXT)
            {
                struct xsql_txt txt = {tok_size(tok), tok_value(tok)};
                if ((rc = xsql_bind_txt(stmt, i, txt))) goto cleanup;
            }
            rc = tok_next(tok, fd);
        }
        assert(tok_id(tok) == TOK_NL);
        rc = xsql_step(stmt);
        if (rc != RC_NEXT)
        {
            rc = error(RC_FAIL, "failed to add prod");
            goto cleanup;
        }
        if ((rc = xsql_end_step(stmt))) goto cleanup;
    } while (true);

cleanup:
    if (rc) return xsql_rollback_transaction(sched_db());
    return xsql_end_transaction(sched_db());
}
