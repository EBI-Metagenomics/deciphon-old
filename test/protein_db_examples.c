#include "protein_db_examples.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include "protein_db.h"

void protein_db_examples_new_ex1(char const *filepath)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct imm_nuclt_code code = {0};
    imm_nuclt_code_init(&code, nuclt);

    FILE *fd = fopen(filepath, "wb");
    NOTNULL(fd);

    struct protein_cfg cfg = protein_cfg(ENTRY_DIST_UNIFORM, 0.1f);
    struct protein_db db = protein_db_default;
    EQ(protein_db_openw(&db, fd, amino, nuclt, cfg), RC_DONE);

    struct protein_profile prof;
    protein_profile_init(&prof, amino, &code, cfg);

    protein_profile_sample(&prof, 1, 2);
    profile_set_name((struct profile *)&prof, metadata("NAME0", "ACC0"));
    EQ(protein_db_write(&db, &prof), RC_DONE);

    protein_profile_sample(&prof, 2, 2);
    profile_set_name((struct profile *)&prof, metadata("NAME1", "ACC1"));
    EQ(protein_db_write(&db, &prof), RC_DONE);

    EQ(db_close((struct db *)&db), RC_DONE);
    fclose(fd);
}
