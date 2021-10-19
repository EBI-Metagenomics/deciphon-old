#include "sql.h"
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
    "    name     VARCHAR (15) PRIMARY KEY"
    "                          UNIQUE"
    "                          NOT NULL,"
    "    size     INTEGER      NOT NULL"
    "                          CONSTRAINT [non-negative] CHECK (size >= 0),"
    "    sym_idx  VARCHAR (77) NOT NULL,"
    "    symbols  VARCHAR (31) NOT NULL,"
    "    creation [DATETIME ]  GENERATED ALWAYS AS (datetime('now') ) "
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
    ");";

enum dcp_rc sql_open(struct dcp_server *srv)
{
    if (sqlite3_open(":memory:", &srv->sql_db))
    {
        sqlite3_close(srv->sql_db);
        return error(DCP_RUNTIMEERROR, "failed to open database");
    }
    return DCP_SUCCESS;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName);

enum dcp_rc sql_create(struct dcp_server *srv)
{
    if (sqlite3_exec(srv->sql_db, schema, callback, 0, NULL) != SQLITE_OK)
        return error(DCP_RUNTIMEERROR, "failed to create database");
    return DCP_SUCCESS;
}

void sql_close(struct dcp_server *srv) { sqlite3_close(srv->sql_db); }

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}
