#include "protein_reader.h"
#include "db_reader.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "expect.h"
#include "fs.h"
#include "partition_size.h"
#include <string.h>

static void cleanup(struct protein_reader *reader)
{
  for (unsigned i = 0; i < reader->npartitions; ++i)
    fclose(lip_file_ptr(reader->file + i));
}

static int open_files(struct protein_reader *reader, FILE *fp)
{
  for (unsigned i = 0; i < reader->npartitions; ++i)
  {
    FILE *f = NULL;
    int rc = fs_refopen(fp, "rb", &f);
    if (rc) return rc;
    lip_file_init(reader->file + i, f);
  }
  return 0;
}

static void init_proteins(struct protein_reader *reader, struct db_reader *db)
{
  for (unsigned i = 0; i < reader->npartitions; ++i)
  {
    struct protein *pro = &reader->proteins[i];
    protein_init(pro, "", &db->amino, &db->code, db->cfg);
  }
}

static void partition_init(struct protein_reader *reader, long offset)
{
  long *poffset = reader->partition_offset;
  unsigned *psize = reader->partition_size;

  memset(poffset, 0, NPARTITIONS_MAX + 1);
  memset(psize, 0, NPARTITIONS_MAX);
  poffset[0] = offset;
}

static void partition_it(struct protein_reader *reader, struct db_reader *db)
{
  unsigned n = db->nproteins;
  unsigned k = reader->npartitions;
  unsigned part = 0;
  for (unsigned i = 0; i < k; ++i)
  {
    long sz = partition_size(n, k, i);
    assert(sz >= 0);
    assert(sz <= UINT_MAX);
    unsigned size = (unsigned)sz;

    reader->partition_size[i] = size;
    for (unsigned j = 0; j < size; ++j)
      reader->partition_offset[i + 1] += db->protein_sizes[part++];

    reader->partition_offset[i + 1] += reader->partition_offset[i];
  }
}

static inline long min(long a, long b) { return a < b ? a : b; }

static int rewind_all(struct protein_reader *);

void protein_reader_init(struct protein_reader *x) { x->npartitions = 0; }

int protein_reader_open(struct protein_reader *reader, struct db_reader *db,
                        unsigned npartitions)
{
  int rc = 0;

  for (unsigned i = 0; i < npartitions; ++i)
    reader->curr_offset[i] = -1;

  if (npartitions == 0) return DCP_EZEROPART;

  if (npartitions > NPARTITIONS_MAX) return DCP_EMANYPARTS;

  reader->npartitions = min(npartitions, db->nproteins);

  if ((rc = expect_map_key(&db->file, "proteins"))) return rc;

  unsigned n = 0;
  if (!lip_read_array_size(&db->file, &n)) return DCP_EFREAD;

  if (n != db->nproteins) return DCP_EFDATA;

  long profiles_offset = 0;
  if ((rc = fs_tell(db->file.fp, &profiles_offset))) return rc;

  if ((rc = open_files(reader, lip_file_ptr(&db->file)))) defer_return(rc);

  init_proteins(reader, (struct db_reader *)db);

  partition_init(reader, profiles_offset);
  partition_it(reader, db);
  if ((rc = rewind_all(reader))) defer_return(rc);

  return rc;

defer:
  cleanup(reader);
  return rc;
}

void protein_reader_close(struct protein_reader *x)
{
  for (unsigned i = 0; i < x->npartitions; ++i)
    protein_del(&x->proteins[i]);
  x->npartitions = 0;
}

unsigned protein_reader_npartitions(struct protein_reader const *reader)
{
  return reader->npartitions;
}

unsigned protein_reader_partition_size(struct protein_reader const *reader,
                                       unsigned partition)
{
  return reader->partition_size[partition];
}

unsigned protein_reader_nprofiles(struct protein_reader const *reader)
{
  unsigned n = 0;
  for (unsigned i = 0; i < reader->npartitions; ++i)
    n += reader->partition_size[i];
  return n;
}

static int rewind_all(struct protein_reader *reader)
{
  for (unsigned i = 0; i < reader->npartitions; ++i)
  {
    FILE *fp = lip_file_ptr(reader->file + i);
    int rc = fs_seek(fp, reader->partition_offset[i], SEEK_SET);
    if (rc) return rc;
  }
  return 0;
}

#if 0
int protein_reader_rewind(struct protein_reader *reader, unsigned partition)
{
  FILE *fp = lip_file_ptr(reader->file + partition);
  return fs_seek(fp, reader->partition_offset[partition], SEEK_SET);
}
#endif

int protein_reader_next(struct protein_reader *reader, unsigned partition,
                        struct protein **protein)
{
  FILE *fp = lip_file_ptr(reader->file + partition);
  int rc = fs_tell(fp, &reader->curr_offset[partition]);
  if (rc) return rc;
  if (protein_reader_end(reader, partition)) return 0;

  *protein = &reader->proteins[partition];
  return protein_unpack(*protein, &reader->file[partition]);
}

bool protein_reader_end(struct protein_reader const *reader, unsigned partition)
{
  return reader->curr_offset[partition] ==
         reader->partition_offset[partition + 1];
}
