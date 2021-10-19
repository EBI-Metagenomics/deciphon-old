#include "sql.h"
#include "dcp.h"
#include "dcp/rc.h"
#include "dcp/server.h"
#include "error.h"
#include "imm/imm.h"
#include "third-party/base64.h"
#include <assert.h>
#include <sqlite3.h>

static_assert(IMM_ABC_MAX_SIZE == 31, "IMM_ABC_MAX_SIZE == 31");
static_assert(IMM_SYM_SIZE == 94, "IMM_SYM_SIZE == 94");

static char const schema[] =
    "--\n"
    "-- File generated with SQLiteStudio v3.3.3 on Tue Oct 19 14:15:37 2021\n"
    "--\n"
    "-- Text encoding used: UTF-8\n"
    "--\n"
    "PRAGMA foreign_keys = off;\n"
    "BEGIN TRANSACTION;\n"
    "\n"
    "-- Table: abc\n"
    "CREATE TABLE abc (\n"
    "    name       VARCHAR (15)  PRIMARY KEY\n"
    "                             UNIQUE\n"
    "                             NOT NULL,\n"
    "    size       INTEGER       NOT NULL\n"
    "                             CONSTRAINT [non-negative] CHECK (size> 0),\n"
    "    sym_idx64  VARCHAR (255) NOT NULL,\n"
    "    symbols    VARCHAR (31)  NOT NULL,\n"
    "    creation   [DATETIME]    GENERATED ALWAYS AS (datetime('now') ),\n"
    "    type       VARCHAR (7)   NOT NULL\n"
    "                             CHECK (type IN ('dna', 'rna', 'amino')),\n"
    "    any_symbol CHAR (1)      NOT NULL\n"
    ");\n"
    "\n"
    "-- Table: target\n"
    "CREATE TABLE target (\n"
    "    sequence VARCHAR (2147483647) NOT NULL,\n"
    "    task     INTEGER              REFERENCES task (id) ON DELETE CASCADE\n"
    "                                  NOT NULL\n"
    ");\n"
    "\n"
    "-- Table: task\n"
    "CREATE TABLE task (\n"
    "    id             INTEGER      PRIMARY KEY AUTOINCREMENT\n"
    "                                UNIQUE\n"
    "                                NOT NULL,\n"
    "    cfg_loglik     BOOLEAN      NOT NULL,\n"
    "    cfg_null       BOOLEAN      NOT NULL,\n"
    "    multiple_hits  BOOLEAN      NOT NULL,\n"
    "    hmmer3_compat  BOOLEAN      NOT NULL,\n"
    "    abc            VARCHAR (15) REFERENCES abc (name)\n"
    "                                NOT NULL,\n"
    "    local_creation DATETIME     NOT NULL\n"
    "                                GENERATED ALWAYS AS (datetime(now))\n"
    ");\n"
    "\n"
    "COMMIT TRANSACTION;\n"
    "PRAGMA foreign_keys = on;\n";

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
    unsigned char const *sym_idx64 =
        base64_encode(abc->sym.idx, IMM_SYM_SIZE, 0);

    char sql[512] = {0};
    int rc = snprintf(
        sql, ARRAY_SIZE(sql),
        "INSERT INTO abc (name, size, sym_idx64, symbols, any_symbol, type) "
        "VALUES ('%s', %d, '%s', '%s', '%c', '%s');",
        name, abc->size, sym_idx64, abc->symbols, abc->any_symbol_id, type);

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
    printf("Schema:\n%s\n", schema);
    if (sqlite3_exec(srv->sql_db, schema, 0, 0, &msg))
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
