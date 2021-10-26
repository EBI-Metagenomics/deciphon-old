#include "jobs.h"
#include "dcp.h"
#include "dcp/job.h"
#include "dcp/jobs.h"
#include "dcp/rc.h"
#include "dcp/server.h"
#include "dcp_file.h"
#include "error.h"
#include "imm/imm.h"
#include "schema.h"
#include "sqldiff.h"
#include "third-party/base64.h"
#include <assert.h>
#include <sqlite3.h>
#include <stdio.h>

#define SQL_SIZE 512

static_assert(IMM_ABC_MAX_SIZE == 31, "IMM_ABC_MAX_SIZE == 31");
static_assert(IMM_SYM_SIZE == 94, "IMM_SYM_SIZE == 94");

static enum dcp_rc check_integrity(char const *filepath, bool *ok);
static enum dcp_rc emerge_db(char const *filepath);
static enum dcp_rc is_empty(char const *filepath, bool *empty);
static enum dcp_rc touch_db(char const *filepath);

static inline enum dcp_rc open_error(sqlite3 *db)
{
    sqlite3_close(db);
    return error(DCP_RUNTIMEERROR, "failed to open jobs database");
}

static inline enum dcp_rc close_error(void)
{
    return error(DCP_RUNTIMEERROR, "failed to close jobs database");
}

static inline enum dcp_rc add_abc_error(void)
{
    return error(DCP_RUNTIMEERROR, "failed to insert abc into jobs database");
}

static inline enum dcp_rc exec_sql(sqlite3 *db, char const sql[SQL_SIZE])
{
    if (sqlite3_exec(db, sql, NULL, NULL, NULL))
        return error(DCP_RUNTIMEERROR, "failed to exec sql");
    return DCP_SUCCESS;
}

enum dcp_rc jobs_add_job(struct dcp_jobs *jobs, unsigned user_id,
                         char const *sid, struct dcp_job *job)
{
    char sql[SQL_SIZE] = {0};

#if 0
    int rc = snprintf(sql, ARRAY_SIZE(sql),
                      "INSERT INTO job "
                      "(sid, multiple_hits, hmmer3_compat, abc, db,"
                      " status, status_log, submission, user)"
                      " VALUES ('%s', '%s', '%s', '%s', %d);",
                      job->sid, job->cfg.multiple_hits, job->cfg.hmmer3_compat,
                      job->abc_id, job->db_id, submission, job->user_id);
#endif
}

enum dcp_rc jobs_add_db(struct dcp_jobs *jobs, unsigned user_id,
                        char const *name, char const *filepath,
                        char const *xxh3, char const *type)
{
    char sql[SQL_SIZE] = {0};

    int rc = snprintf(sql, ARRAY_SIZE(sql),
                      "INSERT INTO db "
                      "(name, filepath, xxh3, type, user)"
                      " VALUES ('%s', '%s', '%s', '%s', %d);",
                      name, filepath, xxh3, type, user_id);

    if (rc < 0) return error(DCP_RUNTIMEERROR, "failed to snprintf");

    return exec_sql(jobs->db, sql);
}

enum dcp_rc jobs_setup(struct dcp_jobs *jobs, char const *filepath)
{
    enum dcp_rc rc = touch_db(filepath);
    if (rc) return rc;

    bool empty = false;
    if ((rc = is_empty(filepath, &empty))) return rc;

    if (empty && (rc = emerge_db(filepath))) return rc;

    bool ok = false;
    if ((rc = check_integrity(filepath, &ok))) return rc;
    if (!ok) return error(DCP_RUNTIMEERROR, "damaged jobs database");

    return rc;
}

enum dcp_rc jobs_open(struct dcp_jobs *jobs, char const *filepath)
{
    if (sqlite3_open(filepath, &jobs->db)) return open_error(jobs->db);
    return DCP_SUCCESS;
}

enum dcp_rc jobs_close(struct dcp_jobs *jobs)
{
    if (sqlite3_close(jobs->db)) return close_error();
    return DCP_SUCCESS;
}

static enum dcp_rc create_ground_truth_db(struct file_tmp *tmp)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if ((rc = file_tmp_mk(tmp))) return rc;
    if ((rc = touch_db(tmp->path))) return rc;
    if ((rc = emerge_db(tmp->path))) return rc;
    return rc;
}

static enum dcp_rc check_integrity(char const *filepath, bool *ok)
{
    struct file_tmp tmp = FILE_TMP_INIT();
    enum dcp_rc rc = DCP_SUCCESS;

    if ((rc = create_ground_truth_db(&tmp))) return rc;
    if ((rc = sqldiff_compare(filepath, tmp.path, ok))) goto cleanup;

cleanup:
    file_tmp_rm(&tmp);
    return rc;
}

/* Unix timestamp */
static char const now[] = "SELECT strftime('%s', 'now')";

static inline enum dcp_rc add_deciphon_user(sqlite3 *db)
{
    return exec_sql(db, "INSERT INTO user (id, username, name, admin) VALUES "
                        "(1, 'deciphon', 'Deciphon', TRUE);");
}

static enum dcp_rc add_abc(sqlite3 *db, struct imm_abc const *abc,
                           char name[static 1], char type[static 1])
{
    unsigned char sym_idx64[BASE64_MAX_SIZE];
    if (!base64_encode(abc->sym.idx, IMM_SYM_SIZE, sym_idx64, 0))
        return error(DCP_RUNTIMEERROR, "failed to encode alphabet");

    char sql[SQL_SIZE] = {0};

    int rc = snprintf(
        sql, ARRAY_SIZE(sql),
        "INSERT INTO abc (name, size, sym_idx64, symbols, any_symbol, type, "
        "creation, user) VALUES ('%s', %d, '%s', '%s', '%c', '%s', (%s), 1);",
        name, abc->size, sym_idx64, abc->symbols, imm_abc_any_symbol(abc), type,
        now);

    if (rc < 0) return error(DCP_RUNTIMEERROR, "failed to snprintf");

    return exec_sql(db, sql);
}

static enum dcp_rc add_alphabets(sqlite3 *db)
{
    enum dcp_rc rc = DCP_SUCCESS;

    if ((rc = add_abc(db, &imm_dna_iupac.super.super, "dna_iupac", "dna")))
        return rc;

    if ((rc = add_abc(db, &imm_rna_iupac.super.super, "rna_iupac", "rna")))
        return rc;

    if ((rc = add_abc(db, &imm_amino_iupac.super, "amino_iupac", "amino")))
        return rc;

    return rc;
}

static enum dcp_rc emerge_db(char const *filepath)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);

    enum dcp_rc rc = DCP_SUCCESS;

    if ((rc = exec_sql(db, (char const *)schema_sql))) return rc;
    if ((rc = add_deciphon_user(db))) return rc;
    if ((rc = add_alphabets(db))) return rc;

    if (sqlite3_close(db)) return close_error();

    return DCP_SUCCESS;
}

static int is_empty_cb(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    return 0;
}

static enum dcp_rc is_empty(char const *filepath, bool *empty)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";

    if (sqlite3_exec(db, sql, is_empty_cb, empty, 0))
    {
        sqlite3_close(db);
        return error(DCP_RUNTIMEERROR, "failed to check if jobs db is empty");
    }

    if (sqlite3_close(db)) return close_error();
    return DCP_SUCCESS;
}

static enum dcp_rc touch_db(char const *filepath)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);
    if (sqlite3_close(db)) return close_error();
    return DCP_SUCCESS;
}
