#include "error.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#define ERROR_PREPARE(n) error(DCP_FAIL, "failed to prepare " n " stmt")

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
static char *ascii_read_one_field(ImportCtx *p)
{
    int c;
    int cSep = p->cColSep;
    int rSep = p->cRowSep;
    p->n = 0;
    c = fgetc(p->in);
    /* if (c == EOF || seenInterrupt) */
    if (c == EOF)
    {
        p->cTerm = EOF;
        return 0;
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

#define ERROR_EXEC(n) error(DCP_FAIL, "failed to exec " n " stmt")

static inline enum dcp_rc begin_transaction(struct sqlite3 *db)
{
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0))
        return ERROR_EXEC("begin");
    return DCP_DONE;
}

static inline enum dcp_rc end_transaction(struct sqlite3 *db)
{
    if (sqlite3_exec(db, "END TRANSACTION;", 0, 0, 0)) return ERROR_EXEC("end");
    return DCP_DONE;
}

static inline int prepare(sqlite3 *db, char const *sql,
                          struct sqlite3_stmt **stmt)
{
    return sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
}

static enum dcp_rc init_ctx(ImportCtx *p, struct sqlite3 *db,
                            char const *filepath)
{
    int nCol;
    int i;
    char *(SQLITE_CDECL * xRead)(ImportCtx *) = ascii_read_one_field;
    memset(p, 0, sizeof(*p));
    p->cColSep = '\t';
    p->cRowSep = '\n';
    p->zFile = filepath;
    p->nLine = 1;
    p->in = fopen(p->zFile, "rb");
    p->xCloser = fclose;
    char const insert_sql[] =
        "INSERT INTO prod (job_id, match_id, seq_id, prof_id, start, end, "
        "abc_id, loglik, null_loglik, model, version, db_id, seq_hash, match) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    struct sqlite3_stmt *insert_stmt = NULL;
    enum dcp_rc rc = DCP_DONE;

    if (prepare(db, insert_sql, &insert_stmt))
    {
        rc = ERROR_PREPARE("submit job");
        goto cleanup;
    }
    nCol = sqlite3_column_count(insert_stmt);

    do
    {
        for (i = 0; i < nCol; i++)
        {
            char *z = xRead(p);
            /*
            ** Did we reach end-of-file before finding any columns?
            ** If so, stop instead of NULL filling the remaining columns.
            */
            if (z == 0 && i == 0) break;
            /*
            ** Did we reach end-of-file OR end-of-line before finding any
            ** columns in ASCII mode?  If so, stop instead of NULL filling
            ** the remaining columns.
            */
            if ((z == 0 || z[0] == 0) && i == 0) break;
            sqlite3_bind_text(insert_stmt, i + 1, z, -1, SQLITE_TRANSIENT);
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
                    sqlite3_bind_null(insert_stmt, i);
                    i++;
                }
            }
        }
        if (p->cTerm == p->cColSep)
        {
            do
            {
                xRead(p);
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
            sqlite3_step(insert_stmt);
            int bla = sqlite3_reset(insert_stmt);
            if (bla != SQLITE_OK)
            {
#if 0
                utf8_printf(stderr, "%s:%d: INSERT failed: %s\n", p->zFile,
                            startLine, sqlite3_errmsg(p->db));
#endif
                p->nErr++;
            }
            else
            {
                p->nRow++;
            }
        }
    } while (p->cTerm != EOF);

cleanup:
    return rc;
}
