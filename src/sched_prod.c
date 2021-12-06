#include "sched_prod.h"
#include "array.h"
#include "logger.h"
#include "macros.h"
#include "protein_match.h"
#include "safe.h"
#include "sched.h"
#include "sched_macros.h"
#include "to.h"
#include "tok.h"
#include "xsql.h"
#include <inttypes.h>
#include <sqlite3.h>
#include <stdlib.h>

#ifdef TAB
#undef TAB
#endif
#define TAB "\t"

#ifdef NL
#undef NL
#endif
#define NL "\n"

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
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum rc sched_prod_add(struct prod *prod)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, prod->job_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, prod->seq_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 3, prod->match_id);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 4, ARRAY_SIZE_OF(*prod, prof_name),
                         prod->prof_name);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 5, ARRAY_SIZE_OF(*prod, abc_name),
                         prod->abc_name);

    BIND_DOUBLE_OR_CLEANUP(rc, stmt, 6, prod->loglik);
    BIND_DOUBLE_OR_CLEANUP(rc, stmt, 7, prod->null_loglik);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 8, ARRAY_SIZE_OF(*prod, prof_typeid),
                         prod->prof_typeid);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 9, ARRAY_SIZE_OF(*prod, version),
                         prod->version);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 10, (int)array_size(prod->match),
                         array_data(prod->match));

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    prod->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

enum rc sched_prod_next(int64_t job_id, int64_t *prod_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT_NEXT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, *prod_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, job_id);

    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return RC_DONE;
    if (code != SQLITE_ROW)
    {
        rc = STEP_ERROR();
        goto cleanup;
    }
    *prod_id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

    return RC_NEXT;

cleanup:
    return rc;
}

enum rc sched_prod_get(struct prod *prod, int64_t prod_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum rc rc = RC_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, prod_id);
    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    prod->id = sqlite3_column_int64(stmt, 0);

    prod->job_id = sqlite3_column_int64(stmt, 1);
    prod->seq_id = sqlite3_column_int64(stmt, 2);
    prod->match_id = sqlite3_column_int64(stmt, 3);

    rc = xsql_get_text(stmt, 4, ARRAY_SIZE_OF(*prod, prof_name),
                       prod->prof_name);
    if (rc) goto cleanup;
    rc = xsql_get_text(stmt, 5, ARRAY_SIZE_OF(*prod, abc_name), prod->abc_name);
    if (rc) goto cleanup;

    prod->loglik = sqlite3_column_double(stmt, 6);
    prod->null_loglik = sqlite3_column_double(stmt, 7);

    rc = xsql_get_text(stmt, 8, ARRAY_SIZE_OF(*prod, prof_typeid),
                       prod->prof_typeid);
    if (rc) goto cleanup;
    rc = xsql_get_text(stmt, 9, ARRAY_SIZE_OF(*prod, version), prod->version);
    if (rc) goto cleanup;

    rc = xsql_get_text_as_array(stmt, 10, &prod->match);
    if (rc) goto cleanup;

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
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
    BEGIN_TRANSACTION_OR_RETURN(sched_db());

    struct sqlite3_stmt *stmt = stmts[INSERT];

    do
    {
        RESET_OR_CLEANUP(rc, stmt);
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
                BIND_INT64_OR_CLEANUP(rc, stmt, i + 1, val);
            }
            else if (col_type[i] == COL_TYPE_DOUBLE)
            {
                double val = 0;
                if (!to_double(tok_value(tok), &val))
                {
                    error(RC_PARSEERROR, "failed to parse double");
                    goto cleanup;
                }
                BIND_DOUBLE_OR_CLEANUP(rc, stmt, i + 1, val);
            }
            else if (col_type[i] == COL_TYPE_TEXT)
            {
                BIND_TEXT_OR_CLEANUP(rc, stmt, i + 1, (int)tok_size(tok),
                                     tok_value(tok));
            }
            rc = tok_next(tok, fd);
        }
        assert(tok_id(tok) == TOK_NL);
        STEP_OR_CLEANUP(stmt, SQLITE_ROW);
        STEP_OR_CLEANUP(stmt, SQLITE_DONE);
    } while (true);

cleanup:
    if (rc)
        ROLLBACK_TRANSACTION(sched_db());
    else
    {
        END_TRANSACTION_OR_RETURN(sched_db());
    }
    return rc;
}
