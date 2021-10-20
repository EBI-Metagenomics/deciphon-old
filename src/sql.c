#include "sql.h"
#include "dcp.h"
#include "dcp/rc.h"
#include "dcp/server.h"
#include "error.h"
#include "imm/imm.h"
#include "schema.h"
#include "third-party/base64.h"
#include <assert.h>
#include <sqlite3.h>

static_assert(IMM_ABC_MAX_SIZE == 31, "IMM_ABC_MAX_SIZE == 31");
static_assert(IMM_SYM_SIZE == 94, "IMM_SYM_SIZE == 94");

enum dcp_rc sql_open(struct dcp_server *srv)
{
    /* if (sqlite3_open(":memory:", &srv->sql_db)) */
    if (sqlite3_open("file:/Users/horta/deciphon.db", &srv->sql_db))
    {
        sqlite3_close(srv->sql_db);
        return error(DCP_RUNTIMEERROR, "failed to open database");
    }
    return DCP_SUCCESS;
}

static char const now_unix_timestamp[] = "SELECT strftime('%%s', 'now')";

static enum dcp_rc add_abc(sqlite3 *db, struct imm_abc const *abc,
                           char name[static 1], char type[static 1])
{
    unsigned char const *sym_idx64 =
        base64_encode(abc->sym.idx, IMM_SYM_SIZE, 0);

    char sql[512] = {0};

    int rc = snprintf(
        sql, ARRAY_SIZE(sql),
        "INSERT INTO abc (name, size, sym_idx64, symbols, any_symbol, type, "
        "creation) VALUES ('%s', %d, '%s', '%s', '%c', '%s', (%s));",
        name, abc->size, sym_idx64, abc->symbols, imm_abc_any_symbol(abc), type,
        now_unix_timestamp);

    if (rc < 0) return error(DCP_RUNTIMEERROR, "failed to insert abc into db");

    char *msg = 0;
    if (sqlite3_exec(db, sql, 0, 0, &msg))
    {
        fprintf(stderr, "SQL error: %s\n", msg);
        return error(DCP_RUNTIMEERROR, "failed to add abc into db");
    }

    return DCP_SUCCESS;
}

enum dcp_rc sql_create(struct dcp_server *srv)
{
    char *msg = 0;

    if (sqlite3_exec(srv->sql_db, (char const *)schema_sql, 0, 0, &msg))
        return error(DCP_RUNTIMEERROR, "failed to create database");

    fprintf(stderr, "SQL error: %s\n", msg);
    struct imm_dna const *dna = &imm_dna_iupac;
    add_abc(srv->sql_db, imm_super(imm_super(dna)), "dna_iupac", "dna");

    return DCP_SUCCESS;
}

void sql_close(struct dcp_server *srv)
{
    int rc = sqlite3_close(srv->sql_db);
    printf("%d\n", rc);
}
