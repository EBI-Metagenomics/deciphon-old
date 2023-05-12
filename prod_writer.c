#include "prod_writer.h"
#include "array_size_field.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "fmt.h"
#include "fs.h"
#include "strkcpy.h"

void prod_writer_init(struct prod_writer *x) { x->nthreads = 0; }

int prod_writer_open(struct prod_writer *x, int nthreads, char const *dir)
{
  if (nthreads > (int)array_size_field(struct prod_writer, threads))
    return DCP_EMANYTHREADS;
  x->nthreads = nthreads;

  if (!strkcpy(x->dirname, dir, array_size_field(struct prod_writer, dirname)))
    return DCP_ELONGPATH;

  int rc = 0;

  char hmmer_dir[DCP_SHORT_PATH_MAX] = {0};
  if ((rc = FMT(hmmer_dir, "%s/hmmer", x->dirname))) return rc;

  if ((rc = fs_mkdir(x->dirname, true))) defer_return(rc);
  if ((rc = fs_mkdir(hmmer_dir, true))) defer_return(rc);

  for (int i = 0; i < nthreads; ++i)
  {
    if ((rc = prod_writer_thrd_init(x->threads + i, i, x->dirname)))
      defer_return(rc);
  }

  return rc;

defer:
  fs_rmdir(hmmer_dir);
  fs_rmdir(x->dirname);
  return rc;
}

int prod_writer_close(struct prod_writer *x)
{
  char filename[DCP_SHORT_PATH_MAX] = {0};
  int rc = 0;

  if ((rc = FMT(filename, "%s/products.tsv", x->dirname))) return rc;

  FILE *fp = fopen(filename, "wb");
  if (!fp) return DCP_EFOPEN;

  bool ok = true;
  ok &= fputs("seq_id\tprofile\tabc\talt\t", fp) >= 0;
  ok &= fputs("null\tevalue\tmatch\n", fp) >= 0;
  if (!ok) defer_return(DCP_EWRITEPROD);

  for (int i = 0; i < x->nthreads; ++i)
  {
    char file[DCP_SHORT_PATH_MAX] = {0};
    if ((rc = FMT(file, "%s/.products.%03d.tsv", x->dirname, i)))
      defer_return(rc);

    FILE *tmp = fopen(file, "rb");
    if (!tmp) defer_return(rc);

    if ((rc = fs_copy(fp, tmp)))
    {
      fclose(tmp);
      defer_return(rc);
    }

    if (fclose(tmp)) defer_return(DCP_EFCLOSE);

    if ((rc = fs_rmfile(file))) defer_return(rc);
  }

  return fclose(fp) ? DCP_EFCLOSE : 0;

defer:
  fclose(fp);
  return rc;
}

struct prod_writer_thrd *prod_writer_thrd(struct prod_writer *x, int idx)
{
  return x->threads + idx;
}
