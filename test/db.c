#include "db_reader.h"
#include "db_writer.h"
#include "hope.h"
#include "imm/imm.h"
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

  struct protein prof = {0};
  protein_init(&prof, "accession0", amino, &code, cfg);

  unsigned core_size = 2;
  protein_sample(&prof, 1, core_size);
  eq(db_writer_pack(&db, &prof), 0);

  protein_sample(&prof, 2, core_size);
  eq(db_writer_pack(&db, &prof), 0);

  protein_del(&prof);
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

  unsigned nprofs = 0;
  struct imm_prod prod = imm_prod();
  int rc = 0;
  struct protein_reader reader = {0};
  eq(protein_reader_setup(&reader, &db, 1), 0);
  struct protein *prof = 0;
  while (!(rc = protein_reader_next(&reader, 0, &prof)))
  {
    if (protein_reader_end(&reader, 0)) break;
    struct imm_task *task = imm_task_new(protein_alt_dp(prof));
    struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
    eq(imm_task_setup(task, &seq), IMM_OK);
    eq(imm_dp_viterbi(protein_alt_dp(prof), task, &prod), IMM_OK);
    close(prod.loglik, logliks[nprofs]);
    imm_del(task);
    ++nprofs;
  }
  eq(rc, 0);
  eq(nprofs, 2);

  imm_del(&prod);
  protein_reader_del(&reader);
  db_reader_close(&db);
  fclose(fp);
}
