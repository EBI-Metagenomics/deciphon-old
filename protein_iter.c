#include "protein_iter.h"
#include "db_reader.h"
#include "fs.h"
#include "protein.h"
#include "protein_reader.h"

void protein_iter_init(struct protein_iter *x, struct protein_reader *reader,
                       int partition, int start_idx, long offset, FILE *fp)
{
  x->partition = partition;
  x->start_idx = start_idx;
  x->curr_idx = start_idx - 1;
  x->offset = offset;
  x->fp = fp;
  lip_file_init(&x->file, fp);
  x->reader = reader;
}

int protein_iter_rewind(struct protein_iter *x)
{
  x->curr_idx = x->start_idx - 1;
  return fs_seek(x->fp, x->offset, SEEK_SET);
}

int protein_iter_next(struct protein_iter *x, struct protein *protein)
{
  x->curr_idx += 1;
  if (protein_iter_end(x)) return 0;
  int rc = protein_unpack(protein, &x->file);
  long offset = 0;
  fs_tell(x->fp, &offset);
  return rc;
}

bool protein_iter_end(struct protein_iter const *x)
{
  int size = protein_reader_partition_size(x->reader, x->partition);
  return x->start_idx + size == x->curr_idx;
}

int protein_iter_idx(struct protein_iter const *x) { return x->curr_idx; }
