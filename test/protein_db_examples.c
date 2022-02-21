#include "protein_db_examples.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include "protein_db.h"

void protein_db_examples_new_ex1(char const *filepath, unsigned core_size)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct imm_nuclt_code code = {0};
    imm_nuclt_code_init(&code, nuclt);

    FILE *fd = fopen(filepath, "wb");
    NOTNULL(fd);

    struct protein_cfg cfg = protein_cfg(ENTRY_DIST_OCCUPANCY, 0.01f);
    struct protein_db db = {0};
    EQ(protein_db_openw(&db, fd, amino, nuclt, cfg), DCP_OK);

    struct protein_profile prof;
    protein_profile_init(&prof, amino, &code, cfg);

    protein_profile_sample(&prof, 1, core_size);
    EQ(db_write_profile((struct db *)&db, (struct profile const *)&prof,
                        metadata("NAME0", "ACC0")),
       DCP_OK);

    protein_profile_sample(&prof, 2, core_size);
    EQ(db_write_profile((struct db *)&db, (struct profile const *)&prof,
                        metadata("NAME1", "ACC1")),
       DCP_OK);

    profile_del((struct profile *)&prof);
    EQ(db_close((struct db *)&db), DCP_OK);
    fclose(fd);
}
