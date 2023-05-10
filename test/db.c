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
  struct imm_nuclt const *nuclt = &imm_dna_iupac.super;
  struct imm_nuclt_code code = {0};
  imm_nuclt_code_init(&code, nuclt);

  FILE *fp = fopen(TMPDIR "/db.dcp", "wb");
  notnull(fp);

  struct db_writer db = {0};
  eq(db_writer_open(&db, fp, amino, nuclt, ENTRY_DIST_OCCUPANCY, 0.01), 0);

  struct protein protein = {0};
  protein_init(&protein, imm_gencode_get(1), amino, &code, ENTRY_DIST_OCCUPANCY,
               0.01);
  protein_set_accession(&protein, "accession0");

  unsigned core_size = 2;
  protein_sample(&protein, 1, core_size);
  eq(db_writer_pack(&db, &protein), 0);

  protein_sample(&protein, 2, core_size);
  eq(db_writer_pack(&db, &protein), 0);

  protein_cleanup(&protein);
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

  struct imm_abc const *abc = &db.nuclt.super;
  eq(abc->typeid, IMM_DNA);

  double logliks[] = {-2720.381428394979, -2849.83007812500};

  unsigned nproteins = 0;
  struct imm_prod prod = imm_prod();
  int rc = 0;
  struct protein_reader reader = {0};
  eq(protein_reader_setup(&reader, &db, 1), 0);
  struct protein_iter it = {0};
  eq(protein_reader_iter(&reader, 0, &it), 0);
  struct protein protein = {0};
  protein_init(&protein, imm_gencode_get(1), &db.amino, &db.code,
               ENTRY_DIST_OCCUPANCY, 0.01);
  while (!(rc = protein_iter_next(&it, &protein)))
  {
    if (protein_iter_end(&it)) break;

    struct imm_task *task = imm_task_new(&protein.alts.full.dp);
    struct imm_seq seq = imm_seq(imm_str(imm_ex2_seq), abc);
    eq(imm_task_setup(task, &seq), 0);
    eq(imm_dp_viterbi(&protein.alts.full.dp, task, &prod), 0);
    close(prod.loglik, logliks[nproteins]);
    imm_task_del(task);
    ++nproteins;
  }
  eq(rc, 0);
  eq(nproteins, 2);

  imm_prod_cleanup(&prod);
  protein_cleanup(&protein);
  db_reader_close(&db);
  fclose(fp);
}
