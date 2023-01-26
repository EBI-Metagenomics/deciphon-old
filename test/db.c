#include "db_reader.h"
#include "db_writer.h"
#include "hope.h"
#include "imm/imm.h"
#include "protein_iter.h"
#include "protein_reader.h"

void test_protein_db_writer(void);
void test_protein_db_reader(void);

int main(void)
{
  test_protein_db_writer();
  test_protein_db_reader();
  return hope_status();
}

void test_protein_db_writer(void)
{
  remove(TMPDIR "/db.dcp");

  struct imm_amino const *amino = &imm_amino_iupac;
  struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
  struct imm_nuclt_code code = {0};
  imm_nuclt_code_init(&code, nuclt);

  FILE *fp = fopen(TMPDIR "/db.dcp", "wb");
  notnull(fp);

  struct cfg cfg = {ENTRY_DIST_OCCUPANCY, 0.01f};
  struct db_writer db = {0};
  eq(db_writer_open(&db, fp, amino, nuclt, cfg), 0);

  struct protein protein = {0};
  protein_init(&protein, "accession0", amino, &code, cfg);

  unsigned core_size = 2;
  protein_sample(&protein, 1, core_size);
  eq(db_writer_pack(&db, &protein), 0);

  protein_sample(&protein, 2, core_size);
  eq(db_writer_pack(&db, &protein), 0);

  protein_del(&protein);
  eq(db_writer_close(&db), 0);
  fclose(fp);
}

void test_protein_db_reader(void)
{
  FILE *fp = fopen(TMPDIR "/db.dcp", "rb");
  notnull(fp);
  struct db_reader db = {0};
  eq(db_reader_open(&db, fp), 0);

  eq(db.nproteins, 2);

  struct imm_abc const *abc = imm_super(&db.nuclt);
  eq(imm_abc_typeid(abc), IMM_DNA);

  double logliks[] = {-2720.381428394979, -2854.53237780213};

  unsigned nproteins = 0;
  struct imm_prod prod = imm_prod();
  int rc = 0;
  struct protein_reader reader = {0};
  eq(protein_reader_setup(&reader, &db, 1), 0);
  struct protein_iter it = {0};
  eq(protein_reader_iter(&reader, 0, &it), 0);
  while (!protein_iter_end(&it))
  {
    if ((rc = protein_iter_next(&it))) break;
    struct protein *protein = protein_iter_get(&it);
    struct imm_task *task = imm_task_new(protein_alt_dp(protein));
    struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
    eq(imm_task_setup(task, &seq), IMM_OK);
    eq(imm_dp_viterbi(protein_alt_dp(protein), task, &prod), IMM_OK);
    close(prod.loglik, logliks[nproteins]);
    imm_del(task);
    ++nproteins;
  }
  eq(rc, 0);
  eq(nproteins, 2);

  imm_del(&prod);
  db_reader_close(&db);
  fclose(fp);
}
