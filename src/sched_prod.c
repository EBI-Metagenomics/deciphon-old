#include "sched_prod.h"
#include "error.h"
#include "macros.h"
#include "pro_match.h"
#include "sched.h"
#include "sched_limits.h"
#include "sched_macros.h"
#include "to.h"
#include "tok.h"
#include "xstrlcpy.h"
#include <sqlite3.h>
#include <stdlib.h>

#define XSIZE(var, member) ARRAY_SIZE(MEMBER_REF(var, member))

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
    NEXT
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO prod\
            (\
                job_id,          seq_id,   match_id,\
                prof_name,     abc_name,            \
                loglik,     null_loglik,            \
                model,          version,            \
                match_data                          \
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
    [NEXT] = \
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

enum dcp_rc sched_prod_module_init(struct sqlite3 *db)
{
    enum dcp_rc rc = DCP_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum dcp_rc sched_prod_add(struct dcp_prod *prod)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, prod->job_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, prod->seq_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 3, prod->match_id);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 4, prod->prof_name);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 5, prod->abc_name);

    BIND_DOUBLE_OR_CLEANUP(rc, stmt, 6, prod->loglik);
    BIND_DOUBLE_OR_CLEANUP(rc, stmt, 7, prod->null_loglik);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 8, prod->model);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 9, prod->version);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 10, prod->match_data);

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    prod->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

enum dcp_rc sched_prod_next(int64_t job_id, int64_t *prod_id)
{
    struct sqlite3_stmt *stmt = stmts[NEXT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, *prod_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, job_id);

    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return DCP_DONE;
    if (code != SQLITE_ROW)
    {
        rc = STEP_ERROR();
        goto cleanup;
    }
    *prod_id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

    return DCP_NEXT;

cleanup:
    return rc;
}

enum dcp_rc sched_prod_get(struct dcp_prod *prod, int64_t prod_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, prod_id);
    STEP_OR_CLEANUP(stmt, SQLITE_ROW);

    prod->id = sqlite3_column_int64(stmt, 0);

    prod->job_id = sqlite3_column_int64(stmt, 1);
    prod->seq_id = sqlite3_column_int64(stmt, 2);
    prod->match_id = sqlite3_column_int64(stmt, 3);

    COLUMN_TEXT(stmt, 4, prod->prof_name, XSIZE(*prod, prof_name));
    COLUMN_TEXT(stmt, 5, prod->abc_name, XSIZE(*prod, abc_name));

    prod->loglik = sqlite3_column_double(stmt, 6);
    prod->null_loglik = sqlite3_column_double(stmt, 7);

    COLUMN_TEXT(stmt, 8, prod->model, XSIZE(*prod, model));
    COLUMN_TEXT(stmt, 9, prod->version, XSIZE(*prod, version));

    COLUMN_TEXT(stmt, 10, prod->match_data, XSIZE(*prod, match_data));

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

void sched_prod_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}

void sched_prod_set_job_id(struct dcp_prod *prod, int64_t job_id)
{
    prod->job_id = job_id;
}

void sched_prod_set_seq_id(struct dcp_prod *prod, int64_t seq_id)
{
    prod->seq_id = seq_id;
}

void sched_prod_set_match_id(struct dcp_prod *prod, int64_t match_id)
{
    prod->match_id = match_id;
}

void sched_prod_set_prof_name(struct dcp_prod *prod,
                              char const prof_name[SCHED_NAME_SIZE])
{
    xstrlcpy(prod->prof_name, prof_name, SCHED_NAME_SIZE);
}

void sched_prod_set_abc_name(struct dcp_prod *prod,
                             char const abc_name[SCHED_SHORT_SIZE])
{
    xstrlcpy(prod->abc_name, abc_name, SCHED_SHORT_SIZE);
}

void sched_prod_set_loglik(struct dcp_prod *prod, double loglik)
{
    prod->loglik = loglik;
}

void sched_prod_set_null_loglik(struct dcp_prod *prod, double null_loglik)
{
    prod->null_loglik = null_loglik;
}

void sched_prod_set_model(struct dcp_prod *prod,
                          char const model[SCHED_SHORT_SIZE])
{
    xstrlcpy(prod->model, model, SCHED_SHORT_SIZE);
}

void sched_prod_set_version(struct dcp_prod *prod,
                            char const version[SCHED_SHORT_SIZE])
{
    xstrlcpy(prod->version, version, SCHED_SHORT_SIZE);
}

#define ERROR_WRITE error(DCP_IOERROR, "failed to write product")

enum dcp_rc sched_prod_write_preamble(struct dcp_prod *p, FILE *restrict fd)
{
    if (fprintf(fd, "%lld" TAB, p->job_id) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%lld" TAB, p->seq_id) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%lld" TAB, p->match_id) < 0) return ERROR_WRITE;

    if (fprintf(fd, "%s" TAB, p->prof_name) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->abc_name) < 0) return ERROR_WRITE;

    /* Reference: https://stackoverflow.com/a/21162120 */
    if (fprintf(fd, "%.17g" TAB, p->loglik) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%.17g" TAB, p->null_loglik) < 0) return ERROR_WRITE;

    if (fprintf(fd, "%s" TAB, p->model) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->version) < 0) return ERROR_WRITE;

    return DCP_DONE;
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

enum dcp_rc sched_prod_write_match(FILE *restrict fd, struct pro_match const *m)
{
    if (fprintf(fd, "%.*s,", m->frag_size, m->frag) < 0)
        return error(DCP_IOERROR, "failed to write frag");

    if (fprintf(fd, "%s,", m->state) < 0)
        return error(DCP_IOERROR, "failed to write state");

    if (fprintf(fd, "%s,", m->codon) < 0)
        return error(DCP_IOERROR, "failed to write codon");

    if (fprintf(fd, "%s", m->amino) < 0)
        return error(DCP_IOERROR, "failed to write amino");

    return DCP_DONE;
}

enum dcp_rc sched_prod_write_match_sep(FILE *restrict fd)
{
    if (fputc(';', fd) == EOF) return error(DCP_IOERROR, "failed to write sep");
    return DCP_DONE;
}

enum dcp_rc sched_prod_add_from_tsv(FILE *restrict fd)
{

    enum dcp_rc rc = DCP_DONE;
    BEGIN_TRANSACTION_OR_RETURN(sched_db());

    struct sqlite3_stmt *stmt = stmts[INSERT];

    struct tok tok = {0};
    tok_init(&tok);

    do
    {
        RESET_OR_CLEANUP(rc, stmt);
        rc = tok_next(&tok, fd);
        if (tok.id == TOK_EOF) break;

        for (int i = 0; i < 10; i++)
        {
            if (col_type[i] == COL_TYPE_INT64)
            {
                int64_t val = 0;
                if (!to_int64(tok.value, &val))
                {
                    error(DCP_PARSEERROR, "failed to parse int64");
                    goto cleanup;
                }
                BIND_INT64_OR_CLEANUP(rc, stmt, i + 1, val);
            }
            else if (col_type[i] == COL_TYPE_DOUBLE)
            {
                double val = 0;
                if (!to_double(tok.value, &val))
                {
                    error(DCP_PARSEERROR, "failed to parse double");
                    goto cleanup;
                }
                BIND_DOUBLE_OR_CLEANUP(rc, stmt, i + 1, val);
            }
            else if (col_type[i] == COL_TYPE_TEXT)
            {
                BIND_TEXT_OR_CLEANUP(rc, stmt, i + 1, tok.value);
            }
            rc = tok_next(&tok, fd);
        }
        assert(tok.id == TOK_NL);
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
