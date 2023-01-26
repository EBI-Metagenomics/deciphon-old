#include "scan_thrd.h"
#include "db_reader.h"
#include "lrt.h"
#include "match.h"
#include "match_iter.h"
#include "prod.h"
#include "prod_thrd.h"
#include "protein.h"
#include "protein_iter.h"
#include "protein_reader.h"

void scan_thrd_init(struct scan_thrd *x, struct protein_reader *reader,
                    int partition, long scan_id)
{
  prod_init(&x->prod);
  struct db_reader const *db = reader->db;
  protein_init(&x->protein, &db->amino, &db->code, db->cfg);
  protein_reader_iter(reader, partition, &x->iter);
  struct imm_abc const *abc = imm_nuclt_super(&db->nuclt);
  prod_set_abc(&x->prod, imm_abc_typeid_name(imm_abc_typeid(abc)));
  prod_set_scan_id(&x->prod, scan_id);
}

void scan_thrd_set_seq_id(struct scan_thrd *x, long seq_id)
{
  prod_set_seq_id(&x->prod, seq_id);
}

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

    // int idx = protein_iter_idx(it);
    // if ((rc = query_hmmer(t, idx, pro))) break;
    struct match match = {0};
    match_init(&match, &x->protein);

    struct match_iter mit = {0};

    match_iter_init(&mit, seq, &alt.prod.path);
    if ((rc = prod_thrd_write(y, &x->prod, &match, &mit))) break;

    match_iter_init(&mit, seq, &alt.prod.path);
    // if ((rc = query_hmmer(y, &x->prod, &match, &mit))) break;

    // if ((rc = write_hmmer(t))) break;
  }

cleanup:
  return rc;
}

#if 0
static int query_hmmer(struct thread *t, int prof_idx,
                       struct match *match, struct match_iter *it)
{
  char *y = NULL;
  int rc = 0;
  while (!(rc = match_iter_next(it, match)))
  {
    if (match_iter_end(it)) break;
    if (!state_is_mute(match->step.state_id)) *y++ = match->amino;
  }




    info("querying hmmer on profidx: %d", prof_idx);
    struct prot_match_iter iter = {0};
    struct prot_match const *match = NULL;

    prot_match_iter(&iter, prof, t->seq, &t->alt.prod.path);
    match = prot_match_iter_next(&iter);

    while ((match = prot_match_iter_next(&iter)))
    {
        char *y = t->amino_sequence;
        while ((match = prot_match_iter_next(&iter)))
        {
            if (!match->mute) *y++ = match->amino;
        }
        *y = '\0';
    }

    int rc = hmmer_client_put(0, prof_idx, t->amino_sequence,
                              hmmer_client_deadline(5000));
    if (rc) efail("hmmerc_put failure: %d", rc);

    rc = hmmer_client_pop(0, t->hmmer_result);
    if (rc) efail("hmmerc_pop failure: %d", rc);

    t->prod.evalue_log = hmmer_result_evalue_ln(t->hmmer_result);
    info("log(e-value): %f", t->prod.evalue_log);
    return 0;
}
#endif

void scan_thrd_cleanup(struct scan_thrd *x) { protein_del(&x->protein); }
