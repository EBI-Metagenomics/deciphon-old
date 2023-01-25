#include "protein_reader.h"
#include "array_size_field.h"
#include "db_reader.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "expect.h"
#include "fs.h"
#include "partition_size.h"
#include <string.h>

void protein_reader_init(struct protein_reader *x)
{
  x->npartitions = 0;
  for (int i = 0; i < (int)array_size_field(struct protein_reader, fp); ++i)
    x->fp[i] = NULL;
}

static int open_files(int size, FILE *fps[], FILE *fp);
static void close_files(int size, FILE *fps[]);
static int rewind_files(struct protein_reader *);

static void partition_init(struct protein_reader *, long offset);
static void partition_it(struct protein_reader *, struct db_reader *);

static inline long min(long a, long b) { return a < b ? a : b; }

int protein_reader_open(struct protein_reader *x, struct db_reader *db,
                        int npartitions)
{
  int rc = 0;

  if (npartitions == 0) defer_return(DCP_EZEROPART);
  if (npartitions > DCP_NPARTITIONS_SIZE) defer_return(DCP_EMANYPARTS);
  x->npartitions = min(npartitions, db->nproteins);

  FILE *fp = lip_file_ptr(&db->file);
  if ((rc = open_files(x->npartitions, x->fp, fp))) defer_return(rc);

  for (int i = 0; i < x->npartitions; ++i)
  {
    protein_init(x->proteins + i, "", &db->amino, &db->code, db->cfg);
    lip_file_init(x->file + i, x->fp[i]);
    x->curr_offset[i] = -1;
  }

  if ((rc = expect_map_key(&db->file, "proteins"))) defer_return(rc);

  unsigned n = 0;
  if (!lip_read_array_size(&db->file, &n)) defer_return(DCP_EFREAD);
  if (n > INT_MAX) defer_return(DCP_EFDATA);
  if ((int)n != db->nproteins) defer_return(DCP_EFDATA);

  long profiles_offset = 0;
  if ((rc = fs_tell(db->file.fp, &profiles_offset))) defer_return(rc);

  partition_init(x, profiles_offset);
  partition_it(x, db);
  if ((rc = rewind_files(x))) defer_return(rc);

  return rc;

defer:
  close_files(x->npartitions, x->fp);
  return rc;
}

void protein_reader_close(struct protein_reader *x)
{
  for (int i = 0; i < x->npartitions; ++i)
    protein_del(&x->proteins[i]);
  x->npartitions = 0;
  close_files(x->npartitions, x->fp);
}

int protein_reader_npartitions(struct protein_reader const *x)
{
  return x->npartitions;
}

int protein_reader_partition_size(struct protein_reader const *x, int partition)
{
  return x->partition_size[partition];
}

int protein_reader_size(struct protein_reader const *x)
{
  int n = 0;
  for (int i = 0; i < x->npartitions; ++i)
    n += x->partition_size[i];
  return n;
}

int protein_reader_rewind(struct protein_reader *x, int partition)
{
  return fs_seek(x->fp[partition], x->partition_offset[partition], SEEK_SET);
}

int protein_reader_next(struct protein_reader *x, int partition,
                        struct protein **protein)
{
  int rc = fs_tell(x->fp[partition], &x->curr_offset[partition]);
  if (rc) return rc;

  if (protein_reader_end(x, partition)) return 0;

  *protein = &x->proteins[partition];
  return protein_unpack(*protein, &x->file[partition]);
}

bool protein_reader_end(struct protein_reader const *x, int partition)
{
  return x->curr_offset[partition] == x->partition_offset[partition + 1];
}

static int open_files(int size, FILE *fps[], FILE *fp)
{
  int rc = 0;

  for (int i = 0; i < size; ++i)
  {
    fps[i] = NULL;
    if ((rc = fs_refopen(fp, "rb", fps + i))) defer_return(rc);
  }

  return rc;

defer:
  close_files(size, fps);
  return rc;
}

static void close_files(int size, FILE *fps[])
{
  for (int i = 0; i < size; ++i)
  {
    if (fps[i])
    {
      fclose(fps[i]);
      fps[i] = NULL;
    }
  }
}

static int rewind_files(struct protein_reader *x)
{
  for (int i = 0; i < x->npartitions; ++i)
  {
    int rc = fs_seek(x->fp[i], x->partition_offset[i], SEEK_SET);
    if (rc) return rc;
  }
  return 0;
}

static void partition_init(struct protein_reader *x, long offset)
{
  long *poffset = x->partition_offset;
  int *psize = x->partition_size;

  memset(poffset, 0, (DCP_NPARTITIONS_SIZE + 1) * sizeof(*poffset));
  memset(psize, 0, DCP_NPARTITIONS_SIZE * sizeof(*psize));
  poffset[0] = offset;
}

static void partition_it(struct protein_reader *x, struct db_reader *db)
{
  int n = db->nproteins;
  int k = x->npartitions;
  int part = 0;
  for (int i = 0; i < k; ++i)
  {
    long sz = partition_size(n, k, i);
    assert(sz >= 0);
    assert(sz <= UINT_MAX);
    int size = (int)sz;

    x->partition_size[i] = size;
    for (int j = 0; j < size; ++j)
      x->partition_offset[i + 1] += db->protein_sizes[part++];

    x->partition_offset[i + 1] += x->partition_offset[i];
  }
}
