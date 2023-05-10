#include "protein.h"
#include "codec.h"
#include "hope.h"
#include "imm/imm.h"

void test_protein_uniform(void);
void test_protein_occupancy(void);

int main(void)
{
  test_protein_uniform();
  test_protein_occupancy();
  return hope_status();
}

void test_protein_uniform(void)
{
  struct imm_amino const *amino = &imm_amino_iupac;
  struct imm_nuclt const *nuclt = &imm_dna_iupac.super;
  struct imm_nuclt_code code;
  imm_nuclt_code_init(&code, nuclt);

  struct protein protein = {0};
  protein_init(&protein, imm_gencode_get(1), amino, &code, ENTRY_DIST_UNIFORM,
               0.1);
  protein_set_accession(&protein, "accession");
  eq(protein_sample(&protein, 1, 2), 0);

  char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
  struct imm_seq seq = imm_seq(IMM_STR(str), protein.imm_code->abc);

  protein_setup(&protein, imm_seq_size(&seq), true, false);

  struct imm_prod prod = imm_prod();
  struct imm_dp *dp = &protein.null.dp;
  struct imm_task *task = imm_task_new(dp);
  notnull(task);
  eq(imm_task_setup(task, &seq), 0);
  eq(imm_dp_viterbi(dp, task, &prod), 0);

  close(prod.loglik, -48.9272687711);

  eq(imm_path_nsteps(&prod.path), 11);
  char name[IMM_STATE_NAME_SIZE];

  eq(imm_path_step(&prod.path, 0)->seqlen, 3);
  eq(imm_path_step(&prod.path, 0)->state_id, STATE_R);
  state_name(imm_path_step(&prod.path, 0)->state_id, name);
  eq(name, "R");

  eq(imm_path_step(&prod.path, 10)->seqlen, 2);
  eq(imm_path_step(&prod.path, 10)->state_id, STATE_R);
  state_name(imm_path_step(&prod.path, 10)->state_id, name);
  eq(name, "R");

  imm_prod_reset(&prod);
  imm_task_del(task);

  dp = &protein.alts.full.dp;
  task = imm_task_new(dp);
  notnull(task);
  eq(imm_task_setup(task, &seq), 0);
  eq(imm_dp_viterbi(dp, task, &prod), 0);

  close(prod.loglik, -55.59428153448);

  eq(imm_path_nsteps(&prod.path), 14);

  eq(imm_path_step(&prod.path, 0)->seqlen, 0);
  eq(imm_path_step(&prod.path, 0)->state_id, STATE_S);
  state_name(imm_path_step(&prod.path, 0)->state_id, name);
  eq(name, "S");

  eq(imm_path_step(&prod.path, 13)->seqlen, 0);
  eq(imm_path_step(&prod.path, 13)->state_id, STATE_T);
  state_name(imm_path_step(&prod.path, 13)->state_id, name);
  eq(name, "T");

  struct codec codec = codec_init(&protein, &prod.path);
  int rc = 0;

  nuclt = protein.nuclt_code->nuclt;
  struct imm_codon codons[10] = {
      IMM_CODON(nuclt, "ATG"), IMM_CODON(nuclt, "AAA"), IMM_CODON(nuclt, "CGC"),
      IMM_CODON(nuclt, "ATA"), IMM_CODON(nuclt, "GCA"), IMM_CODON(nuclt, "CCA"),
      IMM_CODON(nuclt, "CCT"), IMM_CODON(nuclt, "TAC"), IMM_CODON(nuclt, "CAC"),
      IMM_CODON(nuclt, "CAC"),
  };

  unsigned any = imm_abc_any_symbol_id(&nuclt->super);
  struct imm_codon codon = imm_codon(nuclt, any, any, any);
  unsigned i = 0;
  while (!(rc = codec_next(&codec, &seq, &codon)))
  {
    if (codec_end(&codec)) break;
    eq(codons[i].a, codon.a);
    eq(codons[i].b, codon.b);
    eq(codons[i].c, codon.c);
    ++i;
  }
  eq(rc, 0);
  eq(i, 10);

  protein_cleanup(&protein);
  imm_prod_cleanup(&prod);
  imm_task_del(task);
}

void test_protein_occupancy(void)
{
  struct imm_amino const *amino = &imm_amino_iupac;
  struct imm_nuclt const *nuclt = &imm_dna_iupac.super;
  struct imm_nuclt_code code;
  imm_nuclt_code_init(&code, nuclt);

  struct protein protein = {0};
  protein_init(&protein, imm_gencode_get(1), amino, &code, ENTRY_DIST_OCCUPANCY,
               0.1);
  protein_set_accession(&protein, "accession");
  eq(protein_sample(&protein, 1, 2), 0);

  char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
  struct imm_seq seq = imm_seq(imm_str(str), protein.imm_code->abc);

  protein_setup(&protein, imm_seq_size(&seq), true, false);

  struct imm_prod prod = imm_prod();
  struct imm_dp *dp = &protein.null.dp;
  struct imm_task *task = imm_task_new(dp);
  notnull(task);
  eq(imm_task_setup(task, &seq), 0);
  eq(imm_dp_viterbi(dp, task, &prod), 0);

  close(prod.loglik, -48.9272687711);

  eq(imm_path_nsteps(&prod.path), 11);
  char name[IMM_STATE_NAME_SIZE];

  eq(imm_path_step(&prod.path, 0)->seqlen, 3);
  eq(imm_path_step(&prod.path, 0)->state_id, STATE_R);
  state_name(imm_path_step(&prod.path, 0)->state_id, name);
  eq(name, "R");

  eq(imm_path_step(&prod.path, 10)->seqlen, 2);
  eq(imm_path_step(&prod.path, 10)->state_id, STATE_R);
  state_name(imm_path_step(&prod.path, 10)->state_id, name);
  eq(name, "R");

  imm_prod_reset(&prod);
  imm_task_del(task);

  dp = &protein.alts.full.dp;
  task = imm_task_new(dp);
  notnull(task);
  eq(imm_task_setup(task, &seq), 0);
  eq(imm_dp_viterbi(dp, task, &prod), 0);

  close(prod.loglik, -54.35543421312);

  eq(imm_path_nsteps(&prod.path), 14);

  eq(imm_path_step(&prod.path, 0)->seqlen, 0);
  eq(imm_path_step(&prod.path, 0)->state_id, STATE_S);
  state_name(imm_path_step(&prod.path, 0)->state_id, name);
  eq(name, "S");

  eq(imm_path_step(&prod.path, 13)->seqlen, 0);
  eq(imm_path_step(&prod.path, 13)->state_id, STATE_T);
  state_name(imm_path_step(&prod.path, 13)->state_id, name);
  eq(name, "T");

  struct codec codec = codec_init(&protein, &prod.path);
  int rc = 0;

  nuclt = protein.nuclt_code->nuclt;
  struct imm_codon codons[10] = {
      IMM_CODON(nuclt, "ATG"), IMM_CODON(nuclt, "AAA"), IMM_CODON(nuclt, "CGC"),
      IMM_CODON(nuclt, "ATA"), IMM_CODON(nuclt, "GCA"), IMM_CODON(nuclt, "CCA"),
      IMM_CODON(nuclt, "CCT"), IMM_CODON(nuclt, "TAC"), IMM_CODON(nuclt, "CAC"),
      IMM_CODON(nuclt, "CAC"),
  };

  unsigned any = imm_abc_any_symbol_id(&nuclt->super);
  struct imm_codon codon = imm_codon(nuclt, any, any, any);
  unsigned i = 0;
  while (!(rc = codec_next(&codec, &seq, &codon)))
  {
    if (codec_end(&codec)) break;
    eq(codons[i].a, codon.a);
    eq(codons[i].b, codon.b);
    eq(codons[i].c, codon.c);
    ++i;
  }
  eq(rc, 0);
  eq(i, 10);

  protein_cleanup(&protein);
  imm_prod_cleanup(&prod);
  imm_task_del(task);
}
