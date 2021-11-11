#include "sched_prod.h"
#include "error.h"
#include "macros.h"
#include "pro_match.h"
#include "sched_limits.h"
#include "sched_macros.h"
#include "xstrlcpy.h"
#include <sqlite3.h>
#include <stdlib.h>

#define XSIZE(var, member) ARRAY_SIZE(MEMBER_REF(var, member))

enum
{
    INSERT,
    SELECT
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO prod\
            (\
                job_id,          seq_id,   match_id,\
                prof_name,     abc_name,            \
                start_pos,      end_pos,            \
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
                ?, ?,\
                ?\
            ) RETURNING id;\
",
    [SELECT] = "SELECT * FROM seq WHERE id = ?;\
"};
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

enum dcp_rc sched_prod_module_init(struct sqlite3 *db)
{
    enum dcp_rc rc = DCP_DONE;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
        PREPARE_OR_CLEAN_UP(db, queries[i], stmts + i);

cleanup:
    return rc;
}

enum dcp_rc sched_prod_add(struct sched_prod *prod)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    enum dcp_rc rc = DCP_DONE;
    RESET_OR_CLEANUP(rc, stmt);

    BIND_INT64_OR_CLEANUP(rc, stmt, 1, prod->job_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 2, prod->seq_id);
    BIND_INT64_OR_CLEANUP(rc, stmt, 3, prod->match_id);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 4, prod->prof_name);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 5, prod->abc_name);

    BIND_INT64_OR_CLEANUP(rc, stmt, 6, prod->start_pos);
    BIND_INT64_OR_CLEANUP(rc, stmt, 7, prod->end_pos);

    BIND_DOUBLE_OR_CLEANUP(rc, stmt, 8, prod->loglik);
    BIND_DOUBLE_OR_CLEANUP(rc, stmt, 9, prod->null_loglik);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 10, prod->model);
    BIND_TEXT_OR_CLEANUP(rc, stmt, 11, prod->version);

    BIND_TEXT_OR_CLEANUP(rc, stmt, 12, prod->match_data);

    STEP_OR_CLEANUP(stmt, SQLITE_ROW);
    prod->id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = STEP_ERROR();

cleanup:
    return rc;
}

enum dcp_rc sched_prod_get(struct sched_prod *prod, int64_t prod_id)
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

    prod->start_pos = sqlite3_column_int64(stmt, 6);
    prod->end_pos = sqlite3_column_int64(stmt, 7);

    prod->loglik = sqlite3_column_double(stmt, 8);
    prod->null_loglik = sqlite3_column_double(stmt, 9);

    COLUMN_TEXT(stmt, 10, prod->model, XSIZE(*prod, model));
    COLUMN_TEXT(stmt, 11, prod->version, XSIZE(*prod, version));

    COLUMN_TEXT(stmt, 12, prod->match_data, XSIZE(*prod, match_data));

    STEP_OR_CLEANUP(stmt, SQLITE_DONE);

cleanup:
    return rc;
}

void sched_prod_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}

void sched_prod_set_job_id(struct sched_prod *prod, int64_t job_id)
{
    prod->job_id = job_id;
}

void sched_prod_set_seq_id(struct sched_prod *prod, int64_t seq_id)
{
    prod->seq_id = seq_id;
}

void sched_prod_set_match_id(struct sched_prod *prod, int64_t match_id)
{
    prod->match_id = match_id;
}

void sched_prod_set_prof_name(struct sched_prod *prod,
                              char const prof_name[SCHED_NAME_SIZE])
{
    xstrlcpy(prod->prof_name, prof_name, SCHED_NAME_SIZE);
}

void sched_prod_set_start_pos(struct sched_prod *prod, int64_t start_pos)
{
    prod->start_pos = start_pos;
}

void sched_prod_set_end_pos(struct sched_prod *prod, int64_t end_pos)
{
    prod->end_pos = end_pos;
}

void sched_prod_set_abc_name(struct sched_prod *prod,
                             char const abc_name[SCHED_SHORT_SIZE])
{
    xstrlcpy(prod->abc_name, abc_name, SCHED_SHORT_SIZE);
}

void sched_prod_set_loglik(struct sched_prod *prod, double loglik)
{
    prod->loglik = loglik;
}

void sched_prod_set_null_loglik(struct sched_prod *prod, double null_loglik)
{
    prod->null_loglik = null_loglik;
}

void sched_prod_set_model(struct sched_prod *prod,
                          char const model[SCHED_SHORT_SIZE])
{
    xstrlcpy(prod->model, model, SCHED_SHORT_SIZE);
}

void sched_prod_set_version(struct sched_prod *prod,
                            char const version[SCHED_SHORT_SIZE])
{
    xstrlcpy(prod->version, version, SCHED_SHORT_SIZE);
}

#ifdef TAB
#undef TAB
#endif

#define TAB "\t"

#define ERROR_WRITE error(DCP_IOERROR, "failed to write product")

enum dcp_rc sched_prod_write_preamble(struct sched_prod *p, FILE *restrict fd)
{
    if (fprintf(fd, "%lld" TAB, p->job_id) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%lld" TAB, p->seq_id) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%lld" TAB, p->match_id) < 0) return ERROR_WRITE;

    if (fprintf(fd, "%s" TAB, p->prof_name) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->abc_name) < 0) return ERROR_WRITE;

    if (fprintf(fd, "%lld" TAB, p->start_pos) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%lld" TAB, p->end_pos) < 0) return ERROR_WRITE;

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

    if (fprintf(fd, "%c%c%c,", m->codon[0], m->codon[1], m->codon[2]) < 0)
        return error(DCP_IOERROR, "failed to write codon");

    if (fprintf(fd, "%c", m->amino) < 0)
        return error(DCP_IOERROR, "failed to write amino");

    return DCP_DONE;
}

enum dcp_rc sched_prod_write_match_sep(FILE *restrict fd)
{
    if (fputc(';', fd) == EOF) return error(DCP_IOERROR, "failed to write sep");
    return DCP_DONE;
}
