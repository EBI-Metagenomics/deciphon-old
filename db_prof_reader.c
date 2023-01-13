#include "db_prof_reader.h"
#include "db_prot_reader.h"
#include "defer_return.h"
#include "expect.h"
#include "fs.h"
#include "partition_size.h"
#include "rc.h"
#include <string.h>

static void cleanup(struct prof_reader *reader)
{
  for (unsigned i = 0; i < reader->npartitions; ++i)
    fclose(lip_file_ptr(reader->file + i));
}

static int open_files(struct prof_reader *reader, FILE *fp)
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

static void init_prot_profiles(struct prof_reader *reader,
                               struct prot_db_reader *db)
{
  for (unsigned i = 0; i < reader->npartitions; ++i)
  {
    struct prot_prof *pro = &reader->profiles[i].pro;
    prot_prof_init(pro, "", &db->amino, &db->code, db->cfg);
  }
}

static void partition_init(struct prof_reader *reader, long offset)
{
  long *poffset = reader->partition_offset;
  unsigned *psize = reader->partition_size;

  memset(poffset, 0, PARTITIONS_MAX + 1);
  memset(psize, 0, PARTITIONS_MAX);
  poffset[0] = offset;
}

static void partition_it(struct prof_reader *reader, struct db_reader *db)
{
  unsigned n = db->nprofiles;
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
      reader->partition_offset[i + 1] += db->profile_sizes[part++];

    reader->partition_offset[i + 1] += reader->partition_offset[i];
  }
}

static inline long min(long a, long b) { return a < b ? a : b; }

int prof_reader_setup(struct prof_reader *reader, struct db_reader *db,
                      unsigned npartitions)
{
  int rc = 0;

  for (unsigned i = 0; i < npartitions; ++i)
    reader->curr_offset[i] = -1;

  if (npartitions == 0) return EINVAL;

  if (npartitions > PARTITIONS_MAX) return EMANYPARTS;

  reader->npartitions = min(npartitions, db->nprofiles);

  if ((rc = expect_map_key(&db->file, "profiles"))) return rc;

  unsigned n = 0;
  if (!lip_read_array_size(&db->file, &n)) return EFREAD;

  if (n != db->nprofiles) return EFDATA;

  long profiles_offset = 0;
  if ((rc = fs_tell(db->file.fp, &profiles_offset))) return rc;

  reader->prof_typeid = db->prof_typeid;
  if ((rc = open_files(reader, lip_file_ptr(&db->file)))) defer_return(rc);

  if (reader->prof_typeid == PROF_PROT)
    init_prot_profiles(reader, (struct prot_db_reader *)db);
  else
    assert(false && "unknown profile typeid");

  partition_init(reader, profiles_offset);
  partition_it(reader, db);
  if ((rc = prof_reader_rewind_all(reader))) defer_return(rc);

  return rc;

defer:
  cleanup(reader);
  return rc;
}

unsigned prof_reader_npartitions(struct prof_reader const *reader)
{
  return reader->npartitions;
}

unsigned prof_reader_partition_size(struct prof_reader const *reader,
                                    unsigned partition)
{
  return reader->partition_size[partition];
}

unsigned prof_reader_nprofiles(struct prof_reader const *reader)
{
  unsigned n = 0;
  for (unsigned i = 0; i < reader->npartitions; ++i)
    n += reader->partition_size[i];
  return n;
}

int prof_reader_rewind_all(struct prof_reader *reader)
{
  for (unsigned i = 0; i < reader->npartitions; ++i)
  {
    FILE *fp = lip_file_ptr(reader->file + i);
    int rc = fs_seek(fp, reader->partition_offset[i], SEEK_SET);
    if (rc) return rc;
  }
  return 0;
}

int prof_reader_rewind(struct prof_reader *reader, unsigned partition)
{
  FILE *fp = lip_file_ptr(reader->file + partition);
  return fs_seek(fp, reader->partition_offset[partition], SEEK_SET);
}

int prof_reader_next(struct prof_reader *reader, unsigned partition,
                     struct prof **profile)
{
  FILE *fp = lip_file_ptr(reader->file + partition);
  int rc = fs_tell(fp, &reader->curr_offset[partition]);
  if (rc) return rc;
  if (prof_reader_end(reader, partition)) return 0;

  *profile = (struct prof *)&reader->profiles[partition];
  return prof_unpack(*profile, &reader->file[partition]);
}

bool prof_reader_end(struct prof_reader const *reader, unsigned partition)
{
  return reader->curr_offset[partition] ==
         reader->partition_offset[partition + 1];
}

void prof_reader_del(struct prof_reader *reader)
{
  for (unsigned i = 0; i < reader->npartitions; ++i)
    prof_del((struct prof *)&reader->profiles[i]);
}
