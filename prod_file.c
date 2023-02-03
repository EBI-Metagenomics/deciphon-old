#include "prod_file.h"
#include "array_size_field.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "fs.h"
#include <string.h>

void prod_file_init(struct prod_file *x)
{
  x->size = 0;
  for (int i = 0; i < (int)array_size_field(struct prod_file, files); ++i)
    x->files[i] = NULL;
}

int prod_file_setup(struct prod_file *x, int size)
{
  if (size > (int)array_size_field(struct prod_file, prod_threads))
    return DCP_EMANYTHREADS;

  x->size = size;

  for (int i = 0; i < size; ++i)
    x->files[i] = NULL;

  int rc = 0;
  for (int i = 0; i < size; ++i)
  {
    if ((rc = fs_tmpfile(x->files + i))) defer_return(rc);
    prod_thrd_init(x->prod_threads + i, x->files[i]);
  }

  if ((rc = fs_mkdir("prod", true))) defer_return(rc);
  if ((rc = fs_mkdir("prod/hmmer", true))) defer_return(rc);

  return 0;

defer:
  prod_file_cleanup(x);
  return rc;
}

struct prod_thrd *prod_file_thread(struct prod_file *x, int idx)
{
  return x->prod_threads + idx;
}

static int join_files(FILE *dst, int size, FILE *files[]);

int prod_file_finishup(struct prod_file *x, FILE *fp)
{
  if (fputs("scan_id\tseq_id\tprofile_name\t", fp) < 0) return DCP_EWRITEPROD;
  if (fputs("abc_name\talt_loglik\t", fp) < 0) return DCP_EWRITEPROD;
  if (fputs("null_loglik\tevalue_log\t", fp) < 0) return DCP_EWRITEPROD;
  if (fputs("version\tmatch\n", fp) < 0) return DCP_EWRITEPROD;

  int rc = join_files(fp, x->size, x->files);
  prod_file_cleanup(x);
  return rc;
}

void prod_file_cleanup(struct prod_file *x)
{
  for (int i = 0; i < x->size; ++i)
  {
    if (x->files[i]) fclose(x->files[i]);
    x->files[i] = NULL;
  }
  x->size = 0;
}

static int join_files(FILE *dst, int size, FILE *files[])
{
  for (int i = 0; i < size; ++i)
  {
    if (fflush(files[i])) return DCP_EFFLUSH;
    rewind(files[i]);
    int rc = fs_copyp(dst, files[i]);
    if (rc) return rc;
  }
  if (fflush(dst)) return DCP_EFFLUSH;

  return 0;
}
