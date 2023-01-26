#include "scan_thread.h"
#include "lrt.h"
#include "prod.h"
#include "protein.h"
#include "protein_iter.h"

void scan_thread_init(struct scan_thread *x, struct protein_iter *iter)
{
  x->iter = iter;
  x->seq = NULL;
}

void scan_thread_set_sequence(struct scan_thread *x, struct imm_seq const *seq)
{
  x->seq = seq;
}

int scan_thread_run(struct scan_thread *t)
{
  int rc = 0;

  struct protein *proto = 0;
  struct scan_task *null = &t->null;
  struct scan_task *alt = &t->alt;
  struct protein_iter *it = t->iter;
  struct imm_seq const *seq = t->seq;

  if ((rc = protein_iter_rewind(it))) goto cleanup;

  while (!protein_iter_end(it))
  {
    if ((rc = protein_iter_next(it, &proto))) break;
    struct imm_dp const *null_dp = protein_null_dp(proto);
    struct imm_dp const *alt_dp = protein_alt_dp(proto);
    unsigned size = imm_seq_size(seq);

    rc = scan_task_setup(null, protein_null_dp(proto), seq);
    if (rc) goto cleanup;

    rc = scan_task_setup(alt, protein_alt_dp(proto), seq);
    if (rc) goto cleanup;

    rc = protein_setup(proto, size, t->multi_hits, t->hmmer3_compat);
    if (rc) goto cleanup;

    if (imm_dp_viterbi(null_dp, null->task, &null->prod)) goto cleanup;
    if (imm_dp_viterbi(alt_dp, alt->task, &alt->prod)) goto cleanup;

    prod_set_null_loglik(&t->prod, null->prod.loglik);
    prod_set_alt_loglik(&t->prod, alt->prod.loglik);

    // progress_consume(&t->progress, 1);
    imm_float lrt = prod_get_lrt(&t->prod);

    if (!imm_lprob_is_finite(lrt) || lrt < t->lrt_threshold) continue;

    prod_set_protein(&t->prod, proto->accession);

    // int prof_idx = protein_iter_idx(it);
    // if ((rc = query_hmmer(t, prof_idx, pro))) break;
    // if ((rc = write_product(t, pro))) break;
    // if ((rc = write_hmmer(t))) break;
  }

cleanup:
  return rc;
}