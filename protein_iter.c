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
  struct db_reader const *db = reader->db;
  protein_init(&x->protein, "", &db->amino, &db->code, db->cfg);
  x->reader = reader;
}

int protein_iter_rewind(struct protein_iter *x)
{
  x->curr_idx = x->start_idx - 1;
  return fs_seek(x->fp, x->offset, SEEK_SET);
}

int protein_iter_next(struct protein_iter *x)
{
  if (protein_iter_end(x)) return 0;
  x->curr_idx += 1;
  return protein_unpack(&x->protein, &x->file);
}

bool protein_iter_end(struct protein_iter const *x)
{
  return x->start_idx + protein_reader_size(x->reader) == x->curr_idx + 1;
}

struct protein *protein_iter_get(struct protein_iter *x) { return &x->protein; }

int protein_iter_idx(struct protein_iter const *x) { return x->curr_idx; }

void protein_iter_cleanup(struct protein_iter *x) { protein_del(&x->protein); }
