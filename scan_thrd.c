#include "scan_thrd.h"
#include "chararray.h"
#include "db_reader.h"
#include "defer_return.h"
#include "hmmer_dialer.h"
#include "lrt.h"
#include "match.h"
#include "match_iter.h"
#include "prod.h"
#include "prod_thrd.h"
#include "protein.h"
#include "protein_iter.h"
#include "protein_reader.h"

int scan_thrd_init(struct scan_thrd *x, struct protein_reader *reader,
                   int partition, long scan_id, struct hmmer_dialer *dialer)
{
  prod_init(&x->prod);
  struct db_reader const *db = reader->db;
  protein_init(&x->protein, &db->amino, &db->code, db->cfg);
  protein_reader_iter(reader, partition, &x->iter);
  struct imm_abc const *abc = imm_nuclt_super(&db->nuclt);
  prod_set_abc(&x->prod, imm_abc_typeid_name(imm_abc_typeid(abc)));
  prod_set_scan_id(&x->prod, scan_id);
  chararray_init(&x->amino);

  int rc = 0;
  if ((rc = hmmer_init(&x->hmmer))) defer_return(rc);
  if ((rc = hmmer_dialer_dial(dialer, &x->hmmer))) defer_return(rc);

  return rc;

defer:
  protein_del(&x->protein);
  chararray_cleanup(&x->amino);
  return rc;
}

void scan_thrd_cleanup(struct scan_thrd *x)
{
  protein_del(&x->protein);
  chararray_cleanup(&x->amino);
  hmmer_cleanup(&x->hmmer);
}

void scan_thrd_set_seq_id(struct scan_thrd *x, long seq_id)
{
  prod_set_seq_id(&x->prod, seq_id);
}

void scan_thrd_set_lrt_threshold(struct scan_thrd *x, double lrt)
{
  x->lrt_threshold = lrt;
}

void scan_thrd_set_multi_hits(struct scan_thrd *x, bool multihits)
{
  x->multi_hits = multihits;
}

void scan_thrd_set_hmmer3_compat(struct scan_thrd *x, bool h3compat)
{
  x->hmmer3_compat = h3compat;
}

static int infer_amino(struct chararray *x, struct match *match,
                       struct match_iter *it);

int scan_thrd_run(struct scan_thrd *x, struct imm_seq const *seq,
                  struct prod_thrd *y)
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

    struct match match = {0};
    match_init(&match, &x->protein);

    struct match_iter mit = {0};

    match_iter_init(&mit, seq, &alt.prod.path);
    if ((rc = prod_thrd_write(y, &x->prod, &match, &mit))) break;

    match_iter_init(&mit, seq, &alt.prod.path);
    if ((rc = infer_amino(&x->amino, &match, &mit))) break;
    if ((rc = hmmer_put(&x->hmmer, protein_iter_idx(it), x->amino.data))) break;
    if ((rc = hmmer_pop(&x->hmmer))) break;
    if ((rc = prod_thrd_write_hmmer(y, &x->prod, &x->hmmer.result))) break;
  }

cleanup:
  return rc;
}

static int infer_amino(struct chararray *x, struct match *match,
                       struct match_iter *it)
{
  int rc = 0;

  while (!(rc = match_iter_next(it, match)))
  {
    if (match_iter_end(it)) break;
    if (match_state_is_mute(match)) continue;
    if ((rc = chararray_append(x, match_amino(match)))) return rc;
  }

  return chararray_append(x, '\0');
}
