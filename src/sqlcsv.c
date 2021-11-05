#include "error.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

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

static void init_ctx(ImportCtx *p, char const *filepath)
{
    memset(p, 0, sizeof(*p));
    p->cColSep = '\t';
    p->cRowSep = '\n';
    p->zFile = filepath;
    p->nLine = 1;
    p->in = fopen(p->zFile, "rb");
    p->xCloser = fclose;

    do
    {
        int startLine = sCtx.nLine;
        for (i = 0; i < nCol; i++)
        {
            char *z = xRead(&sCtx);
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
            if (p->mode == MODE_Ascii && (z == 0 || z[0] == 0) && i == 0) break;
            sqlite3_bind_text(pStmt, i + 1, z, -1, SQLITE_TRANSIENT);
            if (i < nCol - 1 && sCtx.cTerm != sCtx.cColSep)
            {
                utf8_printf(stderr,
                            "%s:%d: expected %d columns but found %d - "
                            "filling the rest with NULL\n",
                            sCtx.zFile, startLine, nCol, i + 1);
                i += 2;
                while (i <= nCol)
                {
                    sqlite3_bind_null(pStmt, i);
                    i++;
                }
            }
        }
        if (sCtx.cTerm == sCtx.cColSep)
        {
            do
            {
                xRead(&sCtx);
                i++;
            } while (sCtx.cTerm == sCtx.cColSep);
            utf8_printf(stderr,
                        "%s:%d: expected %d columns but found %d - "
                        "extras ignored\n",
                        sCtx.zFile, startLine, nCol, i);
        }
        if (i >= nCol)
        {
            sqlite3_step(pStmt);
            rc = sqlite3_reset(pStmt);
            if (rc != SQLITE_OK)
            {
                utf8_printf(stderr, "%s:%d: INSERT failed: %s\n", sCtx.zFile,
                            startLine, sqlite3_errmsg(p->db));
                sCtx.nErr++;
            }
            else
            {
                sCtx.nRow++;
            }
        }
    } while (sCtx.cTerm != EOF);
}
