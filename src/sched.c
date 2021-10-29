#include "sched.h"
#include "dcp.h"
#include "dcp/job.h"
#include "dcp/seq.h"
#include "dcp/server.h"
#include "dcp/strlcpy.h"
#include "dcp_file.h"
#include "error.h"
#include "imm/imm.h"
#include "sched_job.h"
#include "schema.h"
#include "sqldiff.h"
#include "third-party/base64.h"
#include <assert.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

static_assert(SQLITE_VERSION_NUMBER >= 3035000, "We need RETURNING statement");

#define SQL_SIZE 512

static_assert(IMM_ABC_MAX_SIZE == 31, "IMM_ABC_MAX_SIZE == 31");
static_assert(IMM_SYM_SIZE == 94, "IMM_SYM_SIZE == 94");

static enum dcp_rc check_integrity(char const *filepath, bool *ok);
static enum dcp_rc emerge_db(char const *filepath);
static enum dcp_rc is_empty(char const *filepath, bool *empty);
static enum dcp_rc touch_db(char const *filepath);

static struct
{
    char const *begin;
    struct
    {
        char const *job;
        char const *seq;
    } submit;
    struct
    {
        char const *state;
        char const *pend;
    } job;
    char const *db;
    char const *seq;
    char const *end;
} const sched_stmt = {
    .begin = "BEGIN TRANSACTION;",
    .submit = {.job = " INSERT INTO job (multi_hits, hmmer3_compat, db_id, "
                      "submission) VALUES (?, ?, ?, ?) RETURNING id;",
               .seq = "INSERT INTO seq (data, job_id) VALUES (?, ?);"},
    .job = {.state = "SELECT state FROM job WHERE id = ? LIMIT 1;",
            .pend = "UPDATE job SET state = 'run' WHERE id = (SELECT MIN(id) "
                    "FROM job WHERE state = 'pend') RETURNING id, db_id;"},
    .db = "SELECT filepath FROM db WHERE id = ?;",
    .seq = "SELECT id, data FROM seq WHERE id > ? AND job_id = ? ORDER BY id "
           "ASC LIMIT 1;",
    .end = "COMMIT TRANSACTION;"};

static inline enum dcp_rc open_error(sqlite3 *db)
{
    sqlite3_close(db);
    return error(DCP_FAIL, "failed to open jobs database");
}

static inline enum dcp_rc close_error(void)
{
    return error(DCP_FAIL, "failed to close jobs database");
}

static inline enum dcp_rc exec_sql(sqlite3 *db, char const sql[SQL_SIZE])
{
    if (sqlite3_exec(db, sql, NULL, NULL, NULL))
        return error(DCP_FAIL, "failed to exec sql");
    return DCP_DONE;
}

#define ERROR_PREPARE(n) error(DCP_FAIL, "failed to prepare " n " stmt")
#define ERROR_RESET(n) error(DCP_FAIL, "failed to reset " n " stmt")
#define ERROR_STEP(n) error(DCP_FAIL, "failed to step on " n " stmt")
#define ERROR_EXEC(n) error(DCP_FAIL, "failed to exec " n " stmt")
#define ERROR_BIND(n) error(DCP_FAIL, "failed to bind " n)

static enum dcp_rc begin_transaction(struct sched *sched)
{
    if (sqlite3_reset(sched->stmt.begin)) return ERROR_RESET("begin");
    if (sqlite3_step(sched->stmt.begin) != SQLITE_DONE)
        return ERROR_STEP("begin");
    return DCP_DONE;
}

static enum dcp_rc end_transaction(struct sched *sched)
{
    if (sqlite3_reset(sched->stmt.end)) return ERROR_RESET("end");
    if (sqlite3_step(sched->stmt.end) != SQLITE_DONE) return ERROR_STEP("end");
    return DCP_DONE;
}

enum dcp_rc sched_setup(char const *filepath)
{
    enum dcp_rc rc = touch_db(filepath);
    if (rc) return rc;

    bool empty = false;
    if ((rc = is_empty(filepath, &empty))) return rc;

    if (empty && (rc = emerge_db(filepath))) return rc;

    bool ok = false;
    if ((rc = check_integrity(filepath, &ok))) return rc;
    if (!ok) return error(DCP_FAIL, "damaged jobs database");

    return rc;
}

static void finalize_statements(struct sched *sched)
{
    sqlite3_finalize(sched->stmt.end);
    sqlite3_finalize(sched->stmt.seq);
    sqlite3_finalize(sched->stmt.db);
    sqlite3_finalize(sched->stmt.job.pend);
    sqlite3_finalize(sched->stmt.job.state);
    sqlite3_finalize(sched->stmt.submit.seq);
    sqlite3_finalize(sched->stmt.submit.job);
    sqlite3_finalize(sched->stmt.begin);
}

static int prepare(sqlite3 *db, char const *sql, sqlite3_stmt **stmt)
{
    return sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
}

enum dcp_rc sched_open(struct sched *sched, char const *filepath)
{
    if (sqlite3_open(filepath, &sched->db)) return open_error(sched->db);

    enum dcp_rc rc = DCP_DONE;
    sched->stmt.begin = NULL;
    sched->stmt.submit.job = NULL;
    sched->stmt.submit.seq = NULL;
    sched->stmt.job.state = NULL;
    sched->stmt.end = NULL;

    if (prepare(sched->db, sched_stmt.begin, &sched->stmt.begin))
    {
        rc = ERROR_PREPARE("begin");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.submit.job, &sched->stmt.submit.job))
    {
        rc = ERROR_PREPARE("submit job");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.submit.seq, &sched->stmt.submit.seq))
    {
        rc = ERROR_PREPARE("submit seq");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.job.state, &sched->stmt.job.state))
    {
        rc = ERROR_PREPARE("job state");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.job.pend, &sched->stmt.job.pend))
    {
        rc = ERROR_PREPARE("job pend");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.db, &sched->stmt.db))
    {
        rc = ERROR_PREPARE("db");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.seq, &sched->stmt.seq))
    {
        rc = ERROR_PREPARE("seq");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.end, &sched->stmt.end))
    {
        rc = ERROR_PREPARE("end");
        goto cleanup;
    }

    return rc;

cleanup:
    finalize_statements(sched);
    sqlite3_close(sched->db);
    return rc;
}

enum dcp_rc sched_close(struct sched *sched)
{
    finalize_statements(sched);
    if (sqlite3_close(sched->db)) return close_error();
    return DCP_DONE;
}

static inline sqlite3_int64 columnt_uint64(sqlite3_stmt *stmt, int idx)
{
    sqlite3_int64 val = sqlite3_column_int64(stmt, idx);
    assert(val > 0);
    return val;
}

static enum dcp_rc submit_job(sqlite3_stmt *stmt, struct dcp_job *job,
                              uint64_t db_id, uint64_t *job_id)
{
    enum dcp_rc rc = DCP_DONE;
    if (sqlite3_reset(stmt))
    {
        rc = ERROR_RESET("submit job");
        goto cleanup;
    }
    if (sqlite3_bind_int(stmt, 1, job->multi_hits))
    {
        rc = ERROR_BIND("multi_hits");
        goto cleanup;
    }
    if (sqlite3_bind_int(stmt, 2, job->hmmer3_compat))
    {
        rc = ERROR_BIND("hmmer3_compat");
        goto cleanup;
    }
    if (sqlite3_bind_int64(stmt, 3, (sqlite3_int64)db_id))
    {
        rc = ERROR_BIND("db_id");
        goto cleanup;
    }
    sqlite3_int64 utc = (sqlite3_int64)dcp_utc_now();
    if (sqlite3_bind_int64(stmt, 4, utc))
    {
        rc = ERROR_BIND("submission");
        goto cleanup;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        rc = ERROR_STEP("job");
        goto cleanup;
    }
    *job_id = (uint64_t)columnt_uint64(stmt, 0);

    if (sqlite3_step(stmt) != SQLITE_DONE) rc = ERROR_STEP("job");

cleanup:
    return rc;
}

static enum dcp_rc add_seq(sqlite3_stmt *stmt, char const *seq,
                           sqlite3_int64 job_id)
{
    enum dcp_rc rc = DCP_DONE;
    if (sqlite3_reset(stmt))
    {
        rc = ERROR_RESET("add seq");
        goto cleanup;
    }
    if (sqlite3_bind_text(stmt, 1, seq, -1, NULL))
    {
        rc = ERROR_BIND("seq");
        goto cleanup;
    }
    if (sqlite3_bind_int64(stmt, 2, job_id))
    {
        rc = ERROR_BIND("job_id");
        goto cleanup;
    }
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = ERROR_STEP("seq");

cleanup:
    return rc;
}

enum dcp_rc sched_submit_job(struct sched *sched, struct dcp_job *job,
                             uint64_t db_id, uint64_t *job_id)
{
    enum dcp_rc rc = DCP_DONE;
    if ((rc = begin_transaction(sched))) goto cleanup;

    if ((rc = submit_job(sched->stmt.submit.job, job, db_id, job_id)))
        goto cleanup;

    struct cco_iter iter = cco_queue_iter(&job->seqs);
    struct dcp_seq *seq = NULL;
    cco_iter_for_each_entry(seq, &iter, node)
    {

        sqlite3_int64 id = (sqlite3_int64)*job_id;
        if (add_seq(sched->stmt.submit.seq, seq->data, id)) goto cleanup;
    }
    rc = end_transaction(sched);

cleanup:
    return rc;
}

enum dcp_rc sched_add_db(struct sched *sched, char const *filepath,
                         uint64_t *id)
{
    enum dcp_rc rc = DCP_DONE;
    static char const sql[] =
        "INSERT INTO db (filepath) VALUES (?) RETURNING id;";
    sqlite3_stmt *stmt = NULL;

    if (prepare(sched->db, sql, &stmt))
    {
        rc = ERROR_PREPARE("db");
        goto cleanup;
    }
    if (sqlite3_bind_text(stmt, 1, filepath, -1, NULL))
    {
        rc = ERROR_BIND("filepath");
        goto cleanup;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        rc = ERROR_STEP("db");
        goto cleanup;
    }
    *id = (uint64_t)columnt_uint64(stmt, 0);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        rc = ERROR_STEP("db_id");
        goto cleanup;
    }

cleanup:
    sqlite3_finalize(stmt);
    return rc;
}

enum dcp_rc sched_job_state(struct sched *sched, uint64_t job_id,
                            enum dcp_job_state *state)
{
    enum dcp_rc rc = DCP_DONE;
    if (sqlite3_reset(sched->stmt.job.state))
    {
        rc = ERROR_RESET("job state");
        goto cleanup;
    }

    sqlite3_int64 id = (sqlite3_int64)job_id;
    if (sqlite3_bind_int64(sched->stmt.job.state, 1, id))
    {
        rc = ERROR_BIND("job_id");
        goto cleanup;
    }

    int code = sqlite3_step(sched->stmt.job.state);
    if (code == SQLITE_DONE) return DCP_NOTFOUND;
    if (code != SQLITE_ROW)
    {
        rc = ERROR_STEP("job state");
        goto cleanup;
    }

    *state = (enum dcp_job_state)sqlite3_column_int(sched->stmt.job.state, 1);
    if (sqlite3_step(sched->stmt.job.state) != SQLITE_DONE)
        rc = ERROR_STEP("job state");

cleanup:
    return rc;
}

enum dcp_rc sched_next_pend_job(struct sched *sched, struct dcp_job *job,
                                uint64_t *job_id, uint64_t *db_id)
{
    enum dcp_rc rc = DCP_NEXT;
    if (sqlite3_reset(sched->stmt.job.pend))
    {
        rc = ERROR_RESET("job pend");
        goto cleanup;
    }
    int code = sqlite3_step(sched->stmt.job.pend);
    if (code == SQLITE_DONE) return DCP_DONE;
    if (code != SQLITE_ROW)
    {
        rc = ERROR_STEP("job pend update");
        goto cleanup;
    }
    sqlite3_int64 job_id64 = columnt_uint64(sched->stmt.job.pend, 0);
    *job_id = (uint64_t)job_id64;

    sqlite3_int64 db_id64 = columnt_uint64(sched->stmt.job.pend, 1);
    *db_id = (uint64_t)db_id64;

    if (sqlite3_step(sched->stmt.job.pend) != SQLITE_DONE)
    {
        rc = ERROR_STEP("job pend returning");
        goto cleanup;
    }

cleanup:
    return rc;
}

enum dcp_rc sched_db_filepath(struct sched *sched, uint64_t id,
                              char filepath[FILEPATH_SIZE])
{
    enum dcp_rc rc = DCP_DONE;
    if (sqlite3_reset(sched->stmt.db))
    {
        rc = ERROR_RESET("db");
        goto cleanup;
    }
    if (sqlite3_bind_int64(sched->stmt.db, 1, (sqlite_int64)id))
    {
        rc = ERROR_BIND("db_id");
        goto cleanup;
    }
    if (sqlite3_step(sched->stmt.db) != SQLITE_ROW)
    {
        rc = ERROR_STEP("db");
        goto cleanup;
    }
    char const *fp = (char const *)sqlite3_column_text(sched->stmt.db, 0);
    sqlite3_column_bytes(sched->stmt.db, 0);
    dcp_strlcpy(filepath, fp, FILEPATH_SIZE);
    if (sqlite3_step(sched->stmt.db) != SQLITE_DONE)
    {
        rc = ERROR_STEP("db");
        goto cleanup;
    }

cleanup:
    return rc;
}

enum dcp_rc sched_next_seq(struct sched *sched, uint64_t job_id,
                           uint64_t *seq_id, char seq[5001])
{
    enum dcp_rc rc = DCP_NEXT;
    if (sqlite3_reset(sched->stmt.seq))
    {
        rc = ERROR_RESET("seq");
        goto cleanup;
    }
    if (sqlite3_bind_int64(sched->stmt.seq, 1, (sqlite_int64)*seq_id))
    {
        rc = ERROR_BIND("seq_id");
        goto cleanup;
    }
    if (sqlite3_bind_int64(sched->stmt.seq, 2, (sqlite_int64)job_id))
    {
        rc = ERROR_BIND("job_id");
        goto cleanup;
    }
    int code = sqlite3_step(sched->stmt.seq);
    if (code == SQLITE_DONE) return DCP_DONE;
    if (code != SQLITE_ROW)
    {
        rc = ERROR_STEP("seq");
        goto cleanup;
    }
    sqlite3_int64 id64 = sqlite3_column_int64(sched->stmt.seq, 0);
    *seq_id = (uint64_t)id64;
    char const *data = (char const *)sqlite3_column_text(sched->stmt.seq, 1);
    dcp_strlcpy(seq, data, 5001);
    if (sqlite3_step(sched->stmt.seq) != SQLITE_DONE)
    {
        rc = ERROR_STEP("seq");
        goto cleanup;
    }

cleanup:
    return rc;
}

enum dcp_rc sched_add_result(struct sched *sched, uint64_t job_id,
                             char const *output, char const *codon,
                             char const *amino)
{
    FILE *output_fd = fopen(output, "rb");
    FILE *codon_fd = fopen(codon, "rb");
    FILE *amino_fd = fopen(amino, "rb");

    fseek(output_fd, 0, SEEK_END);
    size_t fsize = (size_t)ftell(output_fd);
    fseek(output_fd, 0, SEEK_SET);
    char *str_output = malloc(fsize + 1);
    fread(str_output, 1, fsize, output_fd);

    fseek(amino_fd, 0, SEEK_END);
    fsize = (size_t)ftell(amino_fd);
    fseek(amino_fd, 0, SEEK_SET);
    char *str_amino = malloc(fsize + 1);
    fread(str_amino, 1, fsize, amino_fd);

    fseek(codon_fd, 0, SEEK_END);
    fsize = (size_t)ftell(codon_fd);
    fseek(codon_fd, 0, SEEK_SET);
    char *str_codon = malloc(fsize + 1);
    fread(str_codon, 1, fsize, codon_fd);

    sqlite3_stmt *stmt = NULL;
    int bla = prepare(sched->db,
                      "INSERT INTO result (job_id, amino_faa, "
                      "codon_fna, output_gff) VALUES (?, ?, ?, ?);",
                      &stmt);
    bla = sqlite3_bind_int64(stmt, 1, (sqlite3_int64)job_id);
    bla = sqlite3_bind_text(stmt, 2, str_amino, -1, NULL);
    bla = sqlite3_bind_text(stmt, 3, str_codon, -1, NULL);
    bla = sqlite3_bind_text(stmt, 4, str_output, -1, NULL);

    bla = sqlite3_step(stmt);

    fclose(amino_fd);
    fclose(codon_fd);
    fclose(output_fd);
    sqlite3_finalize(stmt);
    return DCP_DONE;
}

static enum dcp_rc create_ground_truth_db(struct file_tmp *tmp)
{
    enum dcp_rc rc = DCP_DONE;
    if ((rc = file_tmp_mk(tmp))) return rc;
    if ((rc = touch_db(tmp->path))) return rc;
    if ((rc = emerge_db(tmp->path))) return rc;
    return rc;
}

static enum dcp_rc check_integrity(char const *filepath, bool *ok)
{
    struct file_tmp tmp = FILE_TMP_INIT();
    enum dcp_rc rc = DCP_DONE;

    if ((rc = create_ground_truth_db(&tmp))) return rc;
    if ((rc = sqldiff_compare(filepath, tmp.path, ok))) goto cleanup;

cleanup:
    file_tmp_rm(&tmp);
    return rc;
}

static enum dcp_rc emerge_db(char const *filepath)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);

    enum dcp_rc rc = exec_sql(db, (char const *)schema_sql);
    if (rc) return rc;
    if (sqlite3_close(db)) return close_error();

    return DCP_DONE;
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
        return ERROR_EXEC("is_empty");
    }

    if (sqlite3_close(db)) return close_error();
    return DCP_DONE;
}

static enum dcp_rc touch_db(char const *filepath)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);
    if (sqlite3_close(db)) return close_error();
    return DCP_DONE;
}
