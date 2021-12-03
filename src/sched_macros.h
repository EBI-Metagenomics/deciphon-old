#ifndef SCHED_MACROS_H
#define SCHED_MACROS_H

#define OPEN_ERROR() error(DCP_FAIL, "failed to open database")

#define OPEN_OR_CLEANUP(stmt)                                                  \
    if (sqlite3_reset(stmt))                                                   \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to open database");                       \
        goto cleanup;                                                          \
    }

#define CLOSE_ERROR() error(DCP_FAIL, "failed to close database")

#define RESET_OR_CLEANUP(rc, stmt)                                             \
    if (sqlite3_reset(stmt))                                                   \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to reset statement");                     \
        goto cleanup;                                                          \
    }

#define STEP_ERROR() error(DCP_FAIL, "failed to step")

#define STEP_OR_CLEANUP(stmt, code)                                            \
    if (sqlite3_step(stmt) != code)                                            \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to step");                                \
        goto cleanup;                                                          \
    }

#define EXEC_ERROR() error(DCP_FAIL, "failed to exec statement")

#define BIND_STRING_OR_CLEANUP(rc, stmt, pos, var)                             \
    if (sqlite3_bind_text(stmt, pos, var, -1, SQLITE_TRANSIENT))               \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to bind string");                         \
        goto cleanup;                                                          \
    }

#define BIND_TEXT_OR_CLEANUP(rc, stmt, pos, len, var)                          \
    if (sqlite3_bind_text(stmt, pos, var, len, SQLITE_TRANSIENT))              \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to bind text");                           \
        goto cleanup;                                                          \
    }

#define BIND_INT_OR_CLEANUP(rc, stmt, pos, var)                                \
    if (sqlite3_bind_int(stmt, pos, var))                                      \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to bind int");                            \
        goto cleanup;                                                          \
    }

#define BIND_INT64_OR_CLEANUP(rc, stmt, pos, var)                              \
    if (sqlite3_bind_int64(stmt, pos, var))                                    \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to bind int64");                          \
        goto cleanup;                                                          \
    }

#define BIND_DOUBLE_OR_CLEANUP(rc, stmt, pos, var)                             \
    if (sqlite3_bind_double(stmt, pos, var))                                   \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to bind double");                         \
        goto cleanup;                                                          \
    }

#define EXEC_OR_CLEANUP(db, sql)                                               \
    if (sqlite3_exec(db, sql, 0, 0, 0))                                        \
    {                                                                          \
        rc = EXEC_ERROR();                                                     \
        goto cleanup;                                                          \
    }

#define ROLLBACK_TRANSACTION(db)                                               \
    sqlite3_exec(db, "ROLLBACK TRANSACTION;", 0, 0, 0)

#define BEGIN_TRANSACTION_OR_RETURN(db)                                        \
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0)) return EXEC_ERROR()

#define END_TRANSACTION_OR_RETURN(db)                                          \
    if (sqlite3_exec(db, "END TRANSACTION;", 0, 0, 0)) return EXEC_ERROR()

#define PREPARE_OR_CLEAN_UP(db, sql, stmt)                                     \
    if (sqlite3_prepare_v2(db, sql, -1, (stmt), 0))                            \
    {                                                                          \
        rc = error(DCP_FAIL, "failed to prepare statement");                   \
        goto cleanup;                                                          \
    }

#endif
