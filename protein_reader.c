#include "protein_reader.h"
#include "array_size_field.h"
#include "db_reader.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "expect.h"
#include "fs.h"
#include "partition_size.h"
#include "protein_iter.h"
#include <string.h>

void protein_reader_init(struct protein_reader *x)
{
  x->npartitions = 0;
  memset(x->partition_csum, 0,
         sizeof_field(struct protein_reader, partition_csum));
  memset(x->partition_offset, 0,
         sizeof_field(struct protein_reader, partition_offset));
  x->db = NULL;
}

static void partition_it(struct protein_reader *);
static inline long min(long a, long b) { return a < b ? a : b; }

int protein_reader_setup(struct protein_reader *x, struct db_reader *db,
                         int npartitions)
{
  int rc = 0;
  x->db = db;

  if (npartitions == 0) return DCP_EZEROPART;
  if (npartitions > DCP_NPARTITIONS_MAX) return DCP_EMANYPARTS;
  x->npartitions = min(npartitions, db->nproteins);

  if ((rc = expect_map_key(&db->file, "proteins"))) return rc;

  unsigned n = 0;
  if (!lip_read_array_size(&db->file, &n)) return DCP_EFREAD;
  if (n > INT_MAX) return DCP_EFDATA;
  if ((int)n != db->nproteins) return DCP_EFDATA;

  if ((rc = fs_tell(db->file.fp, x->partition_offset))) return rc;
  partition_it(x);

  return rc;
}

int protein_reader_npartitions(struct protein_reader const *x)
{
  return x->npartitions;
}

int protein_reader_partition_size(struct protein_reader const *x, int partition)
{
  int const *csum = x->partition_csum;
  return csum[partition + 1] - csum[partition];
}

int protein_reader_size(struct protein_reader const *x)
{
  return x->partition_csum[x->npartitions];
}

int protein_reader_iter(struct protein_reader *x, int partition,
                        struct protein_iter *it)
{
  if (partition < 0 || x->npartitions < partition) return DCP_EINVALPART;

  FILE *fp = lip_file_ptr(&x->db->file);
  FILE *newfp = NULL;
  int rc = 0;
  long offset = x->partition_offset[partition];

  if ((rc = fs_refopen(fp, "rb", &newfp))) defer_return(rc);
  if ((rc = fs_seek(newfp, offset, SEEK_SET))) defer_return(rc);

  int start_idx = x->partition_csum[partition];
  protein_iter_init(it, x, partition, start_idx, offset, newfp);

  return rc;

defer:
  if (newfp) fclose(newfp);
  return rc;
}

static void partition_it(struct protein_reader *x)
{
  int n = x->db->nproteins;
  int k = x->npartitions;
  int part = 0;
  for (int i = 0; i < k; ++i)
  {
    int size = (int)partition_size(n, k, i);

    x->partition_csum[i + 1] = x->partition_csum[i] + size;

    for (int j = 0; j < size; ++j)
      x->partition_offset[i + 1] += x->db->protein_sizes[part++];

    x->partition_offset[i + 1] += x->partition_offset[i];
  }
}
