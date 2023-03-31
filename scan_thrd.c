#include "scan_thrd.h"
#include "chararray.h"
#include "db_reader.h"
#include "defer_return.h"
#include "hmmer_dialer.h"
#include "iseq.h"
#include "lrt.h"
#include "match.h"
#include "match_iter.h"
#include "prod_match.h"
#include "prod_writer_thrd.h"
#include "protein.h"
#include "protein_iter.h"
#include "protein_reader.h"

int scan_thrd_init(struct scan_thrd *x, struct protein_reader *reader,
                   int partition, struct prod_writer_thrd *prod_thrd,
                   struct hmmer_dialer *dialer)
{
  struct db_reader const *db = reader->db;
  protein_init(&x->protein, &db->amino, &db->code, db->cfg);
  protein_reader_iter(reader, partition, &x->iter);

  x->prod_thrd = prod_thrd;
  struct imm_abc const *abc = &db->nuclt.super;
  char const *abc_name = imm_abc_typeid_name(abc->typeid);
  prod_match_set_abc(&x->prod_thrd->match, abc_name);

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

int scan_thrd_run(struct scan_thrd *x, struct iseq const *seq)
{
  int rc = 0;

  struct protein_iter *it = &x->iter;
  x->prod_thrd->match.seq_id = seq->id;

  if ((rc = protein_iter_rewind(it))) goto cleanup;

  while (!(rc = protein_iter_next(it, &x->protein)))
  {
    if (protein_iter_end(it)) break;

    struct imm_dp const *null_dp = protein_null_dp(&x->protein);
    struct imm_dp const *alt_dp = protein_alt_dp(&x->protein);

    struct scan_task null = {0};
    struct scan_task alt = {0};

    rc = scan_task_setup(&null, protein_null_dp(&x->protein), seq);
    if (rc) goto cleanup;

    rc = scan_task_setup(&alt, protein_alt_dp(&x->protein), seq);
    if (rc) goto cleanup;

    rc = protein_setup(&x->protein, iseq_size(seq), x->multi_hits,
                       x->hmmer3_compat);
    if (rc) goto cleanup;

    if (imm_dp_viterbi(null_dp, null.task, &null.prod)) goto cleanup;
    if (imm_dp_viterbi(alt_dp, alt.task, &alt.prod)) goto cleanup;

    x->prod_thrd->match.null = null.prod.loglik;
    x->prod_thrd->match.alt = alt.prod.loglik;

    float lrt = prod_match_get_lrt(&x->prod_thrd->match);

    if (!imm_lprob_is_finite(lrt) || lrt < x->lrt_threshold) continue;

    prod_match_set_protein(&x->prod_thrd->match, x->protein.accession);

    struct match match = {0};
    match_init(&match, &x->protein);

    struct match_iter mit = {0};

    match_iter_init(&mit, &seq->iseq, &alt.prod.path);
    if ((rc = infer_amino(&x->amino, &match, &mit))) break;
    if ((rc = hmmer_put(&x->hmmer, protein_iter_idx(it), seq->name,
                        x->amino.data)))
      break;
    if ((rc = hmmer_pop(&x->hmmer))) break;
    x->prod_thrd->match.evalue = hmmer_result_evalue_ln(&x->hmmer.result);
    if ((rc = prod_writer_thrd_put_hmmer(x->prod_thrd, &x->hmmer.result)))
      break;

    match_iter_init(&mit, &seq->iseq, &alt.prod.path);
    if ((rc = prod_writer_thrd_put(x->prod_thrd, &match, &mit))) break;
  }

cleanup:
  return rc;
}

int scan_thrd_run0(struct scan_thrd *x, struct iseq const *seq)
{
  int rc = 0;

  struct protein_iter *it = &x->iter;
  x->prod_thrd->match.seq_id = seq->id;

  if ((rc = protein_iter_rewind(it))) goto cleanup;

  unsigned null_hits = 0;
  unsigned alt0_hits = 0;
  unsigned alt_hits = 0;
  unsigned long long null_msec = 0;
  unsigned long long alt0_msec = 0;
  unsigned long long alt_msec = 0;
  printf("IMPLEMENTACAO VER3\n");
  while (!(rc = protein_iter_next(it, &x->protein)))
  {
    if (protein_iter_end(it)) break;

    struct imm_dp const *null_dp = protein_null_dp(&x->protein);
    struct imm_dp const *alt0_dp = protein_alt0_dp(&x->protein);

    struct scan_task null = {0};
    struct scan_task alt0 = {0};

    rc = scan_task_setup(&null, protein_null_dp(&x->protein), seq);
    if (rc) goto cleanup;

    rc = scan_task_setup(&alt0, protein_alt0_dp(&x->protein), seq);
    if (rc) goto cleanup;

    rc = protein_setup(&x->protein, iseq_size(seq), x->multi_hits,
                       x->hmmer3_compat);
    if (rc) goto cleanup;

    printf("NULL\n");
    if (imm_dp_viterbi(null_dp, null.task, &null.prod)) goto cleanup;
    printf("ALT0\n");
    if (imm_dp_viterbi(alt0_dp, alt0.task, &alt0.prod)) goto cleanup;

    float lrt = lrt(null.prod.loglik, alt0.prod.loglik);

    null_msec += null.prod.mseconds;
    alt0_msec += alt0.prod.mseconds;
    null_hits++;
    alt0_hits++;
    if (!imm_lprob_is_finite(lrt) || lrt < x->lrt_threshold) continue;

    struct imm_dp const *alt_dp = protein_alt_dp(&x->protein);
    struct scan_task alt = {0};

    rc = scan_task_setup(&alt, protein_alt_dp(&x->protein), seq);
    if (rc) goto cleanup;

    if (imm_dp_viterbi(alt_dp, alt.task, &alt.prod)) goto cleanup;

    x->prod_thrd->match.null = null.prod.loglik;
    x->prod_thrd->match.alt = alt.prod.loglik;

    lrt = prod_match_get_lrt(&x->prod_thrd->match);

    alt_msec += alt.prod.mseconds;
    alt_hits++;
    if (!imm_lprob_is_finite(lrt) || lrt < x->lrt_threshold) continue;

    prod_match_set_protein(&x->prod_thrd->match, x->protein.accession);

    struct match match = {0};
    match_init(&match, &x->protein);

    struct match_iter mit = {0};

    match_iter_init(&mit, &seq->iseq, &alt.prod.path);
    if ((rc = infer_amino(&x->amino, &match, &mit))) break;
    if ((rc = hmmer_put(&x->hmmer, protein_iter_idx(it), seq->name,
                        x->amino.data)))
      break;
    if ((rc = hmmer_pop(&x->hmmer))) break;
    x->prod_thrd->match.evalue = hmmer_result_evalue_ln(&x->hmmer.result);
    if ((rc = prod_writer_thrd_put_hmmer(x->prod_thrd, &x->hmmer.result)))
      break;

    match_iter_init(&mit, &seq->iseq, &alt.prod.path);
    if ((rc = prod_writer_thrd_put(x->prod_thrd, &match, &mit))) break;
  }
  printf("%llu,%llu,%llu    %u %u %u\n", null_msec, alt0_msec, alt_msec,
         null_hits, alt0_hits, alt_hits);

cleanup:
  return rc;
}

static int infer_amino(struct chararray *x, struct match *match,
                       struct match_iter *it)
{
  int rc = 0;

  chararray_reset(x);
  while (!(rc = match_iter_next(it, match)))
  {
    if (match_iter_end(it)) break;
    if (match_state_is_mute(match)) continue;
    if ((rc = chararray_append(x, match_amino(match)))) return rc;
  }

  return chararray_append(x, '\0');
}
