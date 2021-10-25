#include "jobs.h"
#include "dcp.h"
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

static bool add_abc(sqlite3 *db, struct imm_abc const *abc, char name[static 1],
                    char type[static 1])
{
    unsigned char const *sym_idx64 =
        base64_encode(abc->sym.idx, IMM_SYM_SIZE, 0);

    char sql[512] = {0};

    int rc = snprintf(
        sql, ARRAY_SIZE(sql),
        "INSERT INTO abc (name, size, sym_idx64, symbols, any_symbol, type, "
        "creation) VALUES ('%s', %d, '%s', '%s', '%c', '%s', (%s));",
        name, abc->size, sym_idx64, abc->symbols, imm_abc_any_symbol(abc), type,
        now);

    if (rc < 0) return false;

    if (sqlite3_exec(db, sql, NULL, NULL, NULL)) return false;

    return true;
}

static enum dcp_rc add_alphabets(sqlite3 *db)
{
    if (!add_abc(db, imm_super(imm_super(&imm_dna_iupac)), "dna_iupac", "dna"))
        return add_abc_error();

    if (!add_abc(db, imm_super(imm_super(&imm_rna_iupac)), "rna_iupac", "rna"))
        return add_abc_error();

    if (!add_abc(db, imm_super(&imm_amino_iupac), "amino_iupac", "amino"))
        return add_abc_error();

    return DCP_SUCCESS;
}

static enum dcp_rc emerge_db(char const *filepath)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);

    if (sqlite3_exec(db, (char const *)schema_sql, NULL, NULL, NULL))
        return error(DCP_RUNTIMEERROR, "failed to insert schema");

    enum dcp_rc rc = add_alphabets(db);
    if (rc) return rc;

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
    static char const *const query = "SELECT name FROM sqlite_master;";

    if (sqlite3_exec(db, query, is_empty_cb, empty, 0))
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
