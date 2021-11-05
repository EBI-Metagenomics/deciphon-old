#include "sched.h"
#include "dcp.h"
#include "dcp/job.h"
#include "dcp/seq.h"
#include "dcp/srv.h"
#include "error.h"
#include "imm/imm.h"
#include "sched_job.h"
#include "schema.h"
#include "sqldiff.h"
#include "xfile.h"
#include "xstrlcpy.h"
#include <assert.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

static_assert(SQLITE_VERSION_NUMBER >= 3035000, "We need RETURNING statement");

static_assert(IMM_ABC_MAX_SIZE == 31, "IMM_ABC_MAX_SIZE == 31");
static_assert(IMM_SYM_SIZE == 94, "IMM_SYM_SIZE == 94");

#define ERROR_OPEN() error(DCP_FAIL, "failed to open jobs database")
#define ERROR_CLOSE() error(DCP_FAIL, "failed to close jobs database")
#define ERROR_PREPARE(n) error(DCP_FAIL, "failed to prepare " n " stmt")
#define ERROR_RESET(n) error(DCP_FAIL, "failed to reset " n " stmt")
#define ERROR_STEP(n) error(DCP_FAIL, "failed to step on " n " stmt")
#define ERROR_EXEC(n) error(DCP_FAIL, "failed to exec " n " stmt")
#define ERROR_BIND(n) error(DCP_FAIL, "failed to bind " n)

static enum dcp_rc add_seq(struct sqlite3_stmt *, char const *seq_id,
                           char const *seq, sqlite3_int64 job_id);
static enum dcp_rc check_integrity(char const *filepath, bool *ok);
static enum dcp_rc create_ground_truth_db(PATH_TEMP_DECLARE(filepath));
static enum dcp_rc emerge_db(char const *filepath);
static enum dcp_rc is_empty(char const *filepath, bool *empty);
static enum dcp_rc submit_job(struct sqlite3_stmt *, struct dcp_job *,
                              int64_t db_id, int64_t *job_id);
static enum dcp_rc touch_db(char const *filepath);

static inline enum dcp_rc begin_transaction(struct sched *sched)
{
    if (sqlite3_exec(sched->db, "BEGIN TRANSACTION;", 0, 0, 0))
        return ERROR_EXEC("begin");
    return DCP_DONE;
}

static inline enum dcp_rc end_transaction(struct sched *sched)
{
    if (sqlite3_exec(sched->db, "END TRANSACTION;", 0, 0, 0))
        return ERROR_EXEC("end");
    return DCP_DONE;
}

static void rollback_transaction(struct sched *sched)
{
    sqlite3_exec(sched->db, "ROLLBACK TRANSACTION;", 0, 0, 0);
}

static inline int prepare(struct sqlite3 *db, char const *sql,
                          struct sqlite3_stmt **stmt)
{
    return sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
}

static struct
{
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
    struct
    {
        char const *select;
        char const *insert;
    } db;
    char const *seq;
    struct
    {
        char const *insert;
    } prod;
} const sched_stmt = {
    .submit = {.job = " INSERT INTO job (multi_hits, hmmer3_compat, db_id, "
                      "submission) VALUES (?, ?, ?, ?) RETURNING id;",
               .seq =
                   "INSERT INTO seq (seq_id, data, job_id) VALUES (?, ?, ?);"},
    .job = {.state = "SELECT state FROM job WHERE id = ? LIMIT 1;",
            .pend = "UPDATE job SET state = 'run' WHERE id = (SELECT MIN(id) "
                    "FROM job WHERE state = 'pend') RETURNING id, db_id;"},
    .db = {.select = "SELECT filepath FROM db WHERE id = ?;",
           .insert = "INSERT INTO db (filepath) VALUES (?) RETURNING id;"},
    .seq = "SELECT id, seq_id, data FROM seq WHERE id > ? AND job_id = ? "
           "ORDER BY id ASC LIMIT 1;",
    .prod =
        {.insert =
             "INSERT INTO prod (job_id, match_id, seq_id, prof_id, start, end, "
             "abc_id, loglik, null_loglik, model, version, db_id, seq_hash, "
             "match) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"},
};

enum dcp_rc sched_setup(char const *filepath)
{
    enum dcp_rc rc = touch_db(filepath);
    if (rc) return rc;

    bool empty = false;
    if ((rc = is_empty(filepath, &empty))) return rc;

    if (empty && (rc = emerge_db(filepath))) return rc;

    bool ok = false;
    if ((rc = check_integrity(filepath, &ok))) return rc;
    if (!ok) return error(DCP_FAIL, "damaged sched database");

    return rc;
}

enum dcp_rc sched_open(struct sched *sched, char const *filepath)
{
    enum dcp_rc rc = DCP_DONE;
    memset(sched, 0, sizeof(*sched));

    if (sqlite3_open(filepath, &sched->db))
    {
        rc = ERROR_OPEN();
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

    if (prepare(sched->db, sched_stmt.db.select, &sched->stmt.db.select))
    {
        rc = ERROR_PREPARE("db select");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.db.insert, &sched->stmt.db.insert))
    {
        rc = ERROR_PREPARE("db insert");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.seq, &sched->stmt.seq))
    {
        rc = ERROR_PREPARE("seq");
        goto cleanup;
    }

    if (prepare(sched->db, sched_stmt.prod.insert, &sched->stmt.prod.insert))
    {
        rc = ERROR_PREPARE("prod insert");
        goto cleanup;
    }

    return rc;

cleanup:
    sqlite3_close(sched->db);
    return rc;
}

enum dcp_rc sched_close(struct sched *sched)
{
    sqlite3_finalize(sched->stmt.prod.insert);
    sqlite3_finalize(sched->stmt.seq);
    sqlite3_finalize(sched->stmt.db.insert);
    sqlite3_finalize(sched->stmt.db.select);
    sqlite3_finalize(sched->stmt.job.pend);
    sqlite3_finalize(sched->stmt.job.state);
    sqlite3_finalize(sched->stmt.submit.seq);
    sqlite3_finalize(sched->stmt.submit.job);
    return sqlite3_close(sched->db) ? ERROR_CLOSE() : DCP_DONE;
}

enum dcp_rc sched_submit_job(struct sched *sched, struct dcp_job *job,
                             int64_t db_id, int64_t *job_id)
{
    enum dcp_rc rc = DCP_DONE;
    if ((rc = begin_transaction(sched))) return rc;

    if ((rc = submit_job(sched->stmt.submit.job, job, db_id, job_id)))
        goto cleanup;

    struct cco_iter iter = cco_queue_iter(&job->seqs);
    struct dcp_seq *seq = NULL;
    cco_iter_for_each_entry(seq, &iter, node)
    {
        if (add_seq(sched->stmt.submit.seq, seq->id, seq->data, *job_id))
            goto cleanup;
    }

cleanup:
    if (rc)
        rollback_transaction(sched);
    else
        rc = end_transaction(sched);
    return rc;
}

enum dcp_rc sched_add_db(struct sched *sched, char const *filepath, int64_t *id)
{
    struct sqlite3_stmt *stmt = sched->stmt.db.insert;
    enum dcp_rc rc = DCP_DONE;
    if (sqlite3_reset(stmt))
    {
        rc = ERROR_RESET("db insert");
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
    *id = sqlite3_column_int64(stmt, 0);

    if (sqlite3_step(stmt) != SQLITE_DONE) rc = ERROR_STEP("db_id");

cleanup:
    return rc;
}

enum dcp_rc sched_job_state(struct sched *sched, int64_t job_id,
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

enum dcp_rc sched_next_job(struct sched *sched, struct dcp_job *job)
{
    enum dcp_rc rc = DCP_NEXT;
    struct sqlite3_stmt *stmt = sched->stmt.job.pend;
    if (sqlite3_reset(stmt))
    {
        rc = ERROR_RESET("job pend");
        goto cleanup;
    }
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return DCP_DONE;
    if (code != SQLITE_ROW)
    {
        rc = ERROR_STEP("job pend update");
        goto cleanup;
    }
    dcp_job_init(job);
    job->id = sqlite3_column_int64(stmt, 0);
    job->db_id = (int64_t)sqlite3_column_int64(stmt, 1);

    if (sqlite3_step(sched->stmt.job.pend) != SQLITE_DONE)
        rc = ERROR_STEP("job pend returning");

cleanup:
    return rc;
}

enum dcp_rc sched_db_filepath(struct sched *sched, int64_t id,
                              char filepath[PATH_SIZE])
{
    enum dcp_rc rc = DCP_DONE;
    struct sqlite3_stmt *stmt = sched->stmt.db.select;
    if (sqlite3_reset(stmt))
    {
        rc = ERROR_RESET("db select");
        goto cleanup;
    }
    if (sqlite3_bind_int64(stmt, 1, id))
    {
        rc = ERROR_BIND("db_id");
        goto cleanup;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        rc = ERROR_STEP("db select");
        goto cleanup;
    }
    char const *fp = (char const *)sqlite3_column_text(stmt, 0);
    sqlite3_column_bytes(stmt, 0);
    xstrlcpy(filepath, fp, PATH_SIZE);

    if (sqlite3_step(stmt) != SQLITE_DONE) rc = ERROR_STEP("db select");

cleanup:
    return rc;
}

enum dcp_rc sched_next_seq(struct sched *sched, int64_t job_id, int64_t *seq_id,
                           struct dcp_seq *seq)
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
    *seq_id = sqlite3_column_int64(sched->stmt.seq, 0);
    char const *id = (char const *)sqlite3_column_text(sched->stmt.seq, 1);
    char const *data = (char const *)sqlite3_column_text(sched->stmt.seq, 2);
    xstrlcpy(seq->id, id, MEMBER_SIZE(*seq, id));
    xstrlcpy(seq->data, data, MEMBER_SIZE(*seq, data));
    if (sqlite3_step(sched->stmt.seq) != SQLITE_DONE) rc = ERROR_STEP("seq");

cleanup:
    return rc;
}

enum dcp_rc sched_add_result(struct sched *sched, int64_t job_id,
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

    struct sqlite3_stmt *stmt = NULL;
    int bla = prepare(sched->db,
                      "INSERT INTO result (job_id, amino_faa, "
                      "codon_fna, output_gff) VALUES (?, ?, ?, ?);",
                      &stmt);
    bla = sqlite3_bind_int64(stmt, 1, (sqlite3_int64)job_id);
    bla = sqlite3_bind_text(stmt, 2, str_amino, -1, NULL);
    bla = sqlite3_bind_text(stmt, 3, str_codon, -1, NULL);
    bla = sqlite3_bind_text(stmt, 4, str_output, -1, NULL);

    bla = sqlite3_step(stmt);
    assert(bla = 0);

    fclose(amino_fd);
    fclose(codon_fd);
    fclose(output_fd);
    sqlite3_finalize(stmt);
    return DCP_DONE;
}

typedef struct ImportCtx ImportCtx;
struct ImportCtx
{
    const char *zFile;      /* Name of the input file */
    FILE *in;               /* Read the CSV text from this input stream */
    int (*xCloser)(FILE *); /* Func to close in */
    char *z;                /* Accumulated text for a field */
    int n;                  /* Number of bytes in z */
    int nAlloc;             /* Space allocated for z[] */
    int nLine;              /* Current line number */
    int nRow;               /* Number of rows imported */
    int nErr;               /* Number of errors encountered */
    int bNotFirst;          /* True if one or more bytes already read */
    int cTerm;   /* Character that terminated the most recent field */
    int cColSep; /* The column separator character.  (Usually ",") */
    int cRowSep; /* The row separator character.  (Usually "\n") */
};

static enum dcp_rc init_ctx(struct sched *sched, ImportCtx *p,
                            struct sqlite3 *db, int64_t job_id,
                            char const *filepath);

enum dcp_rc sched_insert_csv(struct sched *sched, int64_t job_id,
                             char const *filepath)
{
    ImportCtx p = {0};
    enum dcp_rc rc = init_ctx(sched, &p, sched->db, job_id, filepath);
    return rc;
}

static enum dcp_rc add_seq(struct sqlite3_stmt *stmt, char const *seq_id,
                           char const *seq, sqlite3_int64 job_id)
{
    enum dcp_rc rc = DCP_DONE;
    if (sqlite3_reset(stmt))
    {
        rc = ERROR_RESET("add seq");
        goto cleanup;
    }
    if (sqlite3_bind_text(stmt, 1, seq_id, -1, NULL))
    {
        rc = ERROR_BIND("seq_id");
        goto cleanup;
    }
    if (sqlite3_bind_text(stmt, 2, seq, -1, NULL))
    {
        rc = ERROR_BIND("seq");
        goto cleanup;
    }
    if (sqlite3_bind_int64(stmt, 3, job_id))
    {
        rc = ERROR_BIND("job_id");
        goto cleanup;
    }
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = ERROR_STEP("seq");

cleanup:
    return rc;
}

static enum dcp_rc create_ground_truth_db(PATH_TEMP_DECLARE(filepath))
{
    enum dcp_rc rc = DCP_DONE;
    if ((rc = xfile_mktemp(filepath))) return rc;
    if ((rc = touch_db(filepath))) return rc;
    if ((rc = emerge_db(filepath))) return rc;
    return rc;
}

static enum dcp_rc check_integrity(char const *filepath, bool *ok)
{
    PATH_TEMP_DEFINE(tmp);
    enum dcp_rc rc = DCP_DONE;

    if ((rc = create_ground_truth_db(tmp))) return rc;
    if ((rc = sqldiff_compare(filepath, tmp, ok))) goto cleanup;

cleanup:
    remove(tmp);
    return rc;
}

static enum dcp_rc emerge_db(char const *filepath)
{
    struct sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return ERROR_OPEN();

    if (sqlite3_exec(db, (char const *)schema_sql, 0, 0, 0))
    {
        enum dcp_rc rc = ERROR_EXEC("emerge");
        sqlite3_close(db);
        return rc;
    }
    return sqlite3_close(db) ? ERROR_CLOSE() : DCP_DONE;
}

static int is_empty_cb(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    return 0;
}

static enum dcp_rc is_empty(char const *filepath, bool *empty)
{
    struct sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return ERROR_OPEN();

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";
    if (sqlite3_exec(db, sql, is_empty_cb, empty, 0))
    {
        enum dcp_rc rc = ERROR_EXEC("is_empty");
        sqlite3_close(db);
        return rc;
    }

    return sqlite3_close(db) ? ERROR_CLOSE() : DCP_DONE;
}

static enum dcp_rc submit_job(struct sqlite3_stmt *stmt, struct dcp_job *job,
                              int64_t db_id, int64_t *job_id)
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
    *job_id = sqlite3_column_int64(stmt, 0);

    if (sqlite3_step(stmt) != SQLITE_DONE) rc = ERROR_STEP("job");

cleanup:
    return rc;
}

static enum dcp_rc touch_db(char const *filepath)
{
    struct sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return ERROR_OPEN();
    return sqlite3_close(db) ? ERROR_CLOSE() : DCP_DONE;
}

/* Append a single byte to z[] */
static void import_append_char(ImportCtx *p, int c)
{
    if (p->n + 1 >= p->nAlloc)
    {
        p->nAlloc += p->nAlloc + 100;
        p->z = sqlite3_realloc64(p->z, (sqlite3_uint64)p->nAlloc);
        if (p->z == 0) error(DCP_OUTOFMEM, "out of memory for importing csv");
    }
    p->z[p->n++] = (char)c;
}

/* Read a single field of ASCII delimited text.
**
**   +  Input comes from p->in.
**   +  Store results in p->z of length p->n.  Space to hold p->z comes
**      from sqlite3_malloc64().
**   +  Use p->cSep as the column separator.  The default is "\x1F".
**   +  Use p->rSep as the row separator.  The default is "\x1E".
**   +  Keep track of the row number in p->nLine.
**   +  Store the character that terminates the field in p->cTerm.  Store
**      EOF on end-of-file.
**   +  Report syntax errors on stderr
*/
static char *ascii_read_one_field(ImportCtx *p, int *end_of_file)
{
    int c;
    int cSep = p->cColSep;
    int rSep = p->cRowSep;
    *end_of_file = false;
    p->n = 0;
    c = fgetc(p->in);
    /* if (c == EOF || seenInterrupt) */
    if (c == EOF)
    {
        p->cTerm = EOF;
        *end_of_file = true;
        return 0;
    }
    if (c != EOF && c == cSep)
    {
        goto exit_early;
    }
    while (c != EOF && c != cSep && c != rSep)
    {
        import_append_char(p, c);
        c = fgetc(p->in);
    }
    if (c == rSep)
    {
        p->nLine++;
    }
    if (c == EOF)
    {
        *end_of_file = true;
    }
exit_early:
    p->cTerm = c;
    if (p->z) p->z[p->n] = 0;
    return p->z;
}

/* Clean up resourced used by an ImportCtx */
static void import_cleanup(ImportCtx *p)
{
    if (p->in != 0 && p->xCloser != 0)
    {
        p->xCloser(p->in);
        p->in = 0;
    }
    sqlite3_free(p->z);
    p->z = 0;
}

static enum dcp_rc init_ctx(struct sched *sched, ImportCtx *p,
                            struct sqlite3 *db, int64_t job_id,
                            char const *filepath)
{
    int nCol;
    int i;
    int end_of_file = false;
    memset(p, 0, sizeof(*p));
    p->cColSep = '\t';
    p->cRowSep = '\n';
    p->zFile = filepath;
    p->nLine = 1;
    p->in = fopen(p->zFile, "rb");
    p->xCloser = fclose;

    enum dcp_rc rc = DCP_DONE;
    if ((rc = begin_transaction(sched))) return rc;

    struct sqlite3_stmt *stmt = sched->stmt.prod.insert;

    char *z = NULL;

    do
    {
        if (sqlite3_reset(stmt))
        {
            rc = ERROR_RESET("add seq");
            goto cleanup;
        }
        if (sqlite3_bind_int64(stmt, 1, job_id))
        {
            rc = ERROR_BIND("prod job_id");
            goto cleanup;
        }
        nCol = 14;
        for (i = 1; i < nCol; i++)
        {
            z = ascii_read_one_field(p, &end_of_file);
            /*
            ** Did we reach end-of-file before finding any columns?
            ** If so, stop instead of NULL filling the remaining columns.
            */
            if (z == 0 && i == 1) break;
            /*
            ** Did we reach end-of-file OR end-of-line before finding any
            ** columns in ASCII mode?  If so, stop instead of NULL filling
            ** the remaining columns.
            */
            if ((z == 0 || end_of_file) && i == 1) break;
            if (sqlite3_bind_text(stmt, i + 1, z, -1, SQLITE_TRANSIENT))
            {
                rc = ERROR_BIND("prod value");
                goto cleanup;
            }
            if (i < nCol - 1 && p->cTerm != p->cColSep)
            {
#if 0
                utf8_printf(stderr,
                            "%s:%d: expected %d columns but found %d - "
                            "filling the rest with NULL\n",
                            p->zFile, startLine, nCol, i + 1);
#endif
                i += 2;
                while (i <= nCol)
                {
                    if (sqlite3_bind_null(stmt, i))
                    {
                        rc = ERROR_BIND("null");
                        goto cleanup;
                    }
                    i++;
                }
            }
        }
        if (p->cTerm == p->cColSep)
        {
            do
            {
                ascii_read_one_field(p, &end_of_file);
                i++;
            } while (p->cTerm == p->cColSep);
#if 0
            utf8_printf(stderr,
                        "%s:%d: expected %d columns but found %d - "
                        "extras ignored\n",
                        p->zFile, startLine, nCol, i);
#endif
        }
        if (i >= nCol)
        {
            if (sqlite3_step(stmt) != SQLITE_DONE)
            {
                printf("%s\n", sqlite3_errmsg(sched->db));
                rc = ERROR_STEP("insert prod");
                goto cleanup;
            }
            p->nRow++;
        }
    } while (p->cTerm != EOF);

cleanup:
    if (rc)
        rollback_transaction(sched);
    else
        rc = end_transaction(sched);
    free(z);
    return rc;
}
