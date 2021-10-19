#include "sql.h"
#include "dcp.h"
#include "dcp/rc.h"
#include "dcp/server.h"
#include "error.h"
#include "imm/imm.h"
#include <assert.h>
#include <sqlite3.h>

static_assert(IMM_ABC_MAX_SIZE == 31, "IMM_ABC_MAX_SIZE == 31");
static_assert(IMM_SYM_SIZE == 94, "IMM_SYM_SIZE == 94");

static char const schema[] =
    ""
    "--"
    "-- File generated with SQLiteStudio v3.3.3 on Tue Oct 19 14:15:37 2021"
    "--"
    "-- Text encoding used: UTF-8"
    "--"
    "PRAGMA foreign_keys = off;"
    "BEGIN TRANSACTION;"
    ""
    "-- Table: abc"
    "CREATE TABLE abc ("
    "    name       VARCHAR (15) PRIMARY KEY"
    "                            UNIQUE"
    "                            NOT NULL,"
    "    size       INTEGER      NOT NULL"
    "                            CONSTRAINT [non-negative] CHECK (size >= 0),"
    "    sym_idx    CHAR (94)    NOT NULL,"
    "    symbols    VARCHAR (31) NOT NULL,"
    "    creation   [DATETIME]   GENERATED ALWAYS AS (datetime('now') ), "
    "    type       VARCHAR (7)  NOT NULL"
    "                            CHECK (type IN ('dna', 'rna', 'amino') ),"
    "    any_symbol CHAR (1)     NOT NULL"
    ");"
    ""
    ""
    "-- Table: target"
    "CREATE TABLE target ("
    "    sequence VARCHAR (0, 2147483647) NOT NULL,"
    "    task     INTEGER                 REFERENCES task (id) ON DELETE "
    "CASCADE"
    "                                     NOT NULL"
    ");"
    ""
    ""
    "-- Table: task"
    "CREATE TABLE task ("
    "    id             INTEGER      PRIMARY KEY AUTOINCREMENT"
    "                                UNIQUE"
    "                                NOT NULL,"
    "    cfg_loglik     BOOLEAN      NOT NULL,"
    "    cfg_null       BOOLEAN      NOT NULL,"
    "    multiple_hits  BOOLEAN      NOT NULL,"
    "    hmmer3_compat  BOOLEAN      NOT NULL,"
    "    abc            VARCHAR (15) REFERENCES abc (name) "
    "                                NOT NULL,"
    "    local_creation DATETIME     NOT NULL"
    "                                GENERATED ALWAYS AS (datetime(now) )"
    ");"
    ""
    "COMMIT TRANSACTION;"
    "PRAGMA foreign_keys = on;"
    ");";

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

static enum dcp_rc add_abc(sqlite3 *db, struct imm_abc const *abc,
                           char name[static 1], char type[static 1])
{
    char sym_idx[IMM_SYM_SIZE] = {0};
    memcpy(sym_idx, abc->sym.idx, IMM_SYM_SIZE);

    char sql[512] = {0};
    int rc = snprintf(
        sql, ARRAY_SIZE(sql),
        "INSERT INTO abc (name, size, sym_idx, symbols, any_symbol, type) "
        "VALUES ('%s', %d, %.*s, '%s', '%c', '%s');",
        name, abc->size, (int)ARRAY_SIZE(sym_idx), sym_idx, abc->symbols,
        abc->any_symbol_id, type);

    if (rc < 0) return error(DCP_RUNTIMEERROR, "failed to insert abc into db");

    return DCP_SUCCESS;
}

enum dcp_rc sql_create(struct dcp_server *srv)
{
    if (sqlite3_exec(srv->sql_db, schema, 0, 0, 0) != SQLITE_OK)
        return error(DCP_RUNTIMEERROR, "failed to create database");

    struct imm_dna const *dna = &imm_dna_iupac;
    add_abc(srv->sql_db, imm_super(imm_super(dna)), "dna_iupac", "dna");

    if (sqlite3_exec(srv->sql_db, schema, 0, 0, 0) != SQLITE_OK)
        return error(DCP_RUNTIMEERROR, "failed to create database");

    return DCP_SUCCESS;
}

void sql_close(struct dcp_server *srv)
{
    int rc = sqlite3_close(srv->sql_db);
    printf("%d\n", rc);
}
