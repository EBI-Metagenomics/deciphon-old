#include "scan_db.h"
#include "array_size_field.h"
#include "defer_return.h"
#include "strlcpy.h"

void scan_db_init(struct scan_db *x)
{
  x->filename[0] = 0;
  x->fp = NULL;
  db_reader_init(&x->reader);
  protein_reader_init(&x->protein);
}

int scan_db_open(struct scan_db *x, int nthreads)
{
  int rc = 0;

  if (!(x->fp = fopen(x->filename, "rb"))) defer_return(DCP_EOPENDB);

  if ((rc = db_reader_open(&x->reader, x->fp))) defer_return(rc);

  if ((rc = protein_reader_open(&x->protein, &x->reader, nthreads)))
    defer_return(rc);

  return 0;

defer:
  scan_db_close(x);
  return 0;
}

void scan_db_close(struct scan_db *x)
{
  protein_reader_close(&x->protein);
  db_reader_close(&x->reader);
  if (x->fp)
  {
    fclose(x->fp);
    x->fp = NULL;
  }
}

int scan_db_set_filename(struct scan_db *x, char const *filename)
{
  size_t n = array_size_field(struct scan_db, filename);
  return strlcpy(x->filename, filename, n) < n ? 0 : DCP_ELARGEPATH;
}
