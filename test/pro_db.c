#include "dcp/dcp.h"
#include "hope/hope.h"

void test_db_pro_openw(void);
void test_db_pro_openr(void);

int main(void)
{
    test_db_pro_openw();
    test_db_pro_openr();
    return hope_status();
}

void test_db_pro_openw(void)
{
    struct dcp_pro_prof prof;
    dcp_pro_prof_sample(&prof, 1, 2, DCP_ENTRY_DIST_UNIFORM, 0.1f);
    FILE *fd = fopen(TMPDIR "/db.dcp", "wb");
    NOTNULL(fd);

    struct dcp_pro_db *db = dcp_pro_db_openw(fd, prof.cfg);
    NOTNULL(db);
    EQ(dcp_pro_db_write(db, &prof), DCP_ILLEGALARG);

    dcp_prof_nameit(dcp_super(&prof), dcp_meta("Name0", "Acc0"));
    EQ(dcp_pro_db_write(db, &prof), DCP_SUCCESS);
    dcp_del(&prof);

    dcp_pro_prof_sample(&prof, 2, 2, DCP_ENTRY_DIST_UNIFORM, 0.2f);
    dcp_prof_nameit(dcp_super(&prof), dcp_meta("Name1", "Acc1"));
    EQ(dcp_pro_db_write(db, &prof), DCP_ILLEGALARG);
    dcp_del(&prof);

    dcp_pro_prof_sample(&prof, 2, 2, DCP_ENTRY_DIST_OCCUPANCY, 0.1f);
    dcp_prof_nameit(dcp_super(&prof), dcp_meta("Name1", "Acc1"));
    EQ(dcp_pro_db_write(db, &prof), DCP_ILLEGALARG);
    dcp_del(&prof);

    dcp_pro_prof_sample(&prof, 2, 2, DCP_ENTRY_DIST_UNIFORM, 0.1f);
    dcp_prof_nameit(dcp_super(&prof), dcp_meta("Name1", "Acc1"));
    EQ(dcp_pro_db_write(db, &prof), DCP_SUCCESS);
    dcp_del(&prof);

    EQ(dcp_pro_db_close(db), DCP_SUCCESS);
    fclose(fd);
}

void test_db_pro_openr(void)
{
    /* struct dcp_pro_prof prof; */
    /* dcp_pro_prof_sample(&prof, 1, 2, DCP_ENTRY_DIST_UNIFORM, 0.1f); */
    /* struct imm_abc const *abc = dcp_super(&prof)->abc; */
    FILE *fd = fopen(TMPDIR "/db.dcp", "rb");
    NOTNULL(fd);

    /* struct dcp_db *db = dcp_db_openr(fd); */
    /* NOTNULL(db); */
#if 0
    struct dcp_db_cfg cfg = dcp_db_cfg(db);
    EQ(cfg.prof_typeid, DCP_STD_PROFILE);
    EQ(cfg.float_bytes, IMM_FLOAT_BYTES);
    struct imm_abc const *abc = dcp_db_abc(db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(dcp_db_write(db, dcp_super(&prof)), DCP_ILLEGALARG);

    dcp_prof_nameit(dcp_super(&prof), dcp_meta("Name0", "Acc0"));
    EQ(dcp_db_write(db, dcp_super(&prof)), DCP_SUCCESS);
    dcp_del(&prof);

    dcp_pro_prof_sample(&prof, 2, 2, DCP_ENTRY_DIST_UNIFORM, 0.2f);
    dcp_prof_nameit(dcp_super(&prof), dcp_meta("Name1", "Acc1"));
    EQ(dcp_db_write(db, dcp_super(&prof)), DCP_ILLEGALARG);
    dcp_del(&prof);

    dcp_pro_prof_sample(&prof, 2, 2, DCP_ENTRY_DIST_OCCUPANCY, 0.1f);
    dcp_prof_nameit(dcp_super(&prof), dcp_meta("Name1", "Acc1"));
    EQ(dcp_db_write(db, dcp_super(&prof)), DCP_ILLEGALARG);
    dcp_del(&prof);

    dcp_pro_prof_sample(&prof, 2, 2, DCP_ENTRY_DIST_UNIFORM, 0.1f);
    dcp_prof_nameit(dcp_super(&prof), dcp_meta("Name1", "Acc1"));
    EQ(dcp_db_write(db, dcp_super(&prof)), DCP_SUCCESS);
    dcp_del(&prof);

    EQ(dcp_db_close(db), DCP_SUCCESS);
    fclose(fd);
#endif
}
