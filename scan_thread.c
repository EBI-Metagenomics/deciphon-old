#include "scan_thread.h"
#include "db_reader.h"
#include "lrt.h"
#include "match.h"
#include "match_iter.h"
#include "prod.h"
#include "protein.h"
#include "protein_iter.h"
#include "protein_reader.h"

void scan_thread_init(struct scan_thread *x, struct protein_reader *reader,
                      int partition)
{
  struct db_reader const *db = reader->db;
  protein_init(&x->protein, &db->amino, &db->code, db->cfg);
  protein_reader_iter(reader, partition, &x->iter);
}

int scan_thread_run(struct scan_thread *x, struct imm_seq const *seq)
{
  int rc = 0;

  struct protein_iter *it = &x->iter;

  if ((rc = protein_iter_rewind(it))) goto cleanup;

  while (!(rc = protein_iter_next(it, &x->protein)))
  {
    if (protein_iter_end(it)) break;

    struct imm_dp const *null_dp = protein_null_dp(&x->protein);
    struct imm_dp const *alt_dp = protein_alt_dp(&x->protein);
    unsigned size = imm_seq_size(seq);

    struct scan_task null = {0};
    struct scan_task alt = {0};

    rc = scan_task_setup(&null, protein_null_dp(&x->protein), seq);
    if (rc) goto cleanup;

    rc = scan_task_setup(&alt, protein_alt_dp(&x->protein), seq);
    if (rc) goto cleanup;

    rc = protein_setup(&x->protein, size, x->multi_hits, x->hmmer3_compat);
    if (rc) goto cleanup;

    if (imm_dp_viterbi(null_dp, null.task, &null.prod)) goto cleanup;
    if (imm_dp_viterbi(alt_dp, alt.task, &alt.prod)) goto cleanup;

    prod_set_null_loglik(&x->prod, null.prod.loglik);
    prod_set_alt_loglik(&x->prod, alt.prod.loglik);

    // progress_consume(&t->progress, 1);
    imm_float lrt = prod_get_lrt(&x->prod);

    if (!imm_lprob_is_finite(lrt) || lrt < x->lrt_threshold) continue;

    prod_set_protein(&x->prod, x->protein.accession);

    int idx = protein_iter_idx(it);
    // if ((rc = query_hmmer(t, prof_idx, pro))) break;
    // if ((rc = write_product(t, pro, seq, &x->alt.prod.path))) break;
    // if ((rc = write_hmmer(t))) break;
  }

cleanup:
  return rc;
}

void scan_thread_cleanup(struct scan_thread *x) { protein_del(&x->protein); }

#if 0
static int write_product(struct prod *prod, struct protein const *protein,
                         struct imm_seq const *seq, struct imm_path const *path,
                         FILE *fp)
{
  int rc = 0;

  if ((rc = prod_write_begin(prod, fp))) return rc;

  struct match_iter it = {0};
  match_iter_init(&it, protein, seq, path);

  if (match_iter_end(&it)) return prod_write_end(fp);

  goto enter;
  while (!match_iter_end(&it))
  {
    if ((rc = match_iter_next(&it))) return rc;
    if ((rc = prod_write_sep(fp))) return rc;
  enter:
    if ((rc = match_write(match_iter_get(&it), fp))) return rc;
  }

  return prod_write_end(fp);
}
#endif
