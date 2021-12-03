#include "protein_db_examples.h"
#include "dcp/dcp.h"
#include "hope/hope.h"

void protein_db_examples_new_ex1(char const *filepath)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);

    FILE *fd = fopen(filepath, "wb");
    NOTNULL(fd);

    struct protein_cfg cfg = protein_cfg(DCP_ENTRY_DIST_UNIFORM, 0.1f);
    struct protein_db db = protein_db_default;
    EQ(protein_db_openw(&db, fd, amino, nuclt, cfg), DCP_DONE);

    struct protein_prof *prof = protein_db_profile(&db);

    protein_profile_sample(prof, 1, 2);
    dcp_prof_nameit(dcp_super(prof), meta("NAME0", "ACC0"));
    EQ(protein_db_write(&db, prof), DCP_DONE);

    protein_profile_sample(prof, 2, 2);
    dcp_prof_nameit(dcp_super(prof), meta("NAME1", "ACC1"));
    EQ(protein_db_write(&db, prof), DCP_DONE);

    EQ(protein_db_close(&db), DCP_DONE);
    fclose(fd);
}
