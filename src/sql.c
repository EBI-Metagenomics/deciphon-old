#include "sql.h"
#include "dcp.h"
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

    if (rc < 0) return error(DCP_RUNTIMEERROR, "failed to insert abc into db");

    char *msg = 0;
    if (sqlite3_exec(db, sql, 0, 0, &msg))
    {
        fprintf(stderr, "SQL error: %s\n", msg);
        return false;
    }

    return true;
}

static int is_empty_callback(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    return 0;
}

static enum dcp_rc is_empty(sqlite3 *db, bool *empty)
{
    *empty = true;
    char const *query = "SELECT name FROM sqlite_master;";

    if (sqlite3_exec(db, query, is_empty_callback, empty, 0))
        return error(DCP_RUNTIMEERROR, "failed to check if database is empty");

    return DCP_SUCCESS;
}

static enum dcp_rc add_alphabets(sqlite3 *db)
{
    add_abc(db, imm_super(imm_super(&imm_dna_iupac)), "dna_iupac", "dna");
    add_abc(db, imm_super(imm_super(&imm_rna_iupac)), "rna_iupac", "rna");
    add_abc(db, imm_super(&imm_amino_iupac), "amino_iupac", "amino");
    return DCP_SUCCESS;
}

static enum dcp_rc init_db(sqlite3 *db)
{
    char *msg = 0;

    if (sqlite3_exec(db, (char const *)schema_sql, 0, 0, &msg))
        return error(DCP_RUNTIMEERROR, "failed to insert schema");

    enum dcp_rc rc = add_alphabets(db);
    if (rc) return rc;

    return DCP_SUCCESS;
}

static enum dcp_rc create_db(char const *filepath)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db))
    {
        sqlite3_close(db);
        return error(DCP_RUNTIMEERROR, "failed to open database");
    }

    if (sqlite3_close(db))
        return error(DCP_RUNTIMEERROR, "failed to close database");

    return DCP_SUCCESS;
}

static enum dcp_rc create_temporary_db(struct file_tmp *tmp)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if ((rc = file_tmp_mk(tmp))) return rc;
    if ((rc = create_db(tmp->path))) return rc;
    return rc;
}

enum dcp_rc sql_setup(struct dcp_server *srv, char const *filepath)
{
    if (sqlite3_open(filepath, &srv->sql_db))
    {
        sqlite3_close(srv->sql_db);
        return error(DCP_RUNTIMEERROR, "failed to open database");
    }

    enum dcp_rc rc = DCP_SUCCESS;
    bool empty = false;
    if ((rc = is_empty(srv->sql_db, &empty))) return rc;

    if (sqlite3_close(srv->sql_db))
        return error(DCP_RUNTIMEERROR, "failed to close database");

    if (empty)
    {
        if ((rc = create_db(filepath))) return rc;
    }

    struct file_tmp tmp = FILE_TMP_INIT();
    if ((rc = create_temporary_db(&tmp))) return rc;

    sqldiff_compare(filepath, tmp.path);
    file_tmp_rm(&tmp);

    return DCP_SUCCESS;
}

void sql_close(struct dcp_server *srv)
{
    int rc = sqlite3_close(srv->sql_db);
    printf("%d\n", rc);
}
