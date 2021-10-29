#include "pro_db_examples.h"
#include "dcp/dcp.h"
#include "hope/hope.h"

void pro_db_examples_new_ex1(char const *filepath)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);

    FILE *fd = fopen(filepath, "wb");
    NOTNULL(fd);

    struct dcp_pro_cfg cfg = dcp_pro_cfg(DCP_ENTRY_DIST_UNIFORM, 0.1f);
    struct dcp_pro_db db = dcp_pro_db_default;
    EQ(dcp_pro_db_openw(&db, fd, amino, nuclt, cfg), DCP_DONE);

    struct dcp_pro_prof *prof = dcp_pro_db_profile(&db);

    dcp_pro_prof_sample(prof, 1, 2);
    dcp_prof_nameit(dcp_super(prof), dcp_meta("NAME0", "ACC0"));
    EQ(dcp_pro_db_write(&db, prof), DCP_DONE);

    dcp_pro_prof_sample(prof, 2, 2);
    dcp_prof_nameit(dcp_super(prof), dcp_meta("NAME1", "ACC1"));
    EQ(dcp_pro_db_write(&db, prof), DCP_DONE);

    EQ(dcp_pro_db_close(&db), DCP_DONE);
    fclose(fd);
}
