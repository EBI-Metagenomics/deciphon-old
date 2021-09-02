#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_db_pro_openw(void);

int main(void)
{
    test_db_pro_openw();
    return hope_status();
}

void test_db_pro_openw(void)
{
    struct dcp_pro_prof p;
    dcp_pro_prof_sample(&p, 1, 2, DCP_ENTRY_DIST_UNIFORM, 0.1f);
    struct imm_abc const *abc = dcp_super(&p)->abc;
    FILE *fd = fopen(TMPDIR "/db.dcp", "wb");

    struct dcp_db *db = dcp_db_openw(fd, abc, dcp_db_pro(p.cfg));
    NOTNULL(db);
    EQ(dcp_db_write(db, dcp_super(&p)), DCP_ILLEGALARG);

    dcp_prof_nameit(dcp_super(&p), dcp_meta("Name0", "Acc0"));
    EQ(dcp_db_write(db, dcp_super(&p)), DCP_SUCCESS);
    dcp_del(&p);

    dcp_pro_prof_sample(&p, 2, 2, DCP_ENTRY_DIST_UNIFORM, 0.2f);
    dcp_prof_nameit(dcp_super(&p), dcp_meta("Name1", "Acc1"));
    EQ(dcp_db_write(db, dcp_super(&p)), DCP_ILLEGALARG);
    dcp_del(&p);

    dcp_pro_prof_sample(&p, 2, 2, DCP_ENTRY_DIST_OCCUPANCY, 0.1f);
    dcp_prof_nameit(dcp_super(&p), dcp_meta("Name1", "Acc1"));
    EQ(dcp_db_write(db, dcp_super(&p)), DCP_ILLEGALARG);
    dcp_del(&p);

    dcp_pro_prof_sample(&p, 2, 2, DCP_ENTRY_DIST_UNIFORM, 0.1f);
    dcp_prof_nameit(dcp_super(&p), dcp_meta("Name1", "Acc1"));
    EQ(dcp_db_write(db, dcp_super(&p)), DCP_SUCCESS);
    dcp_del(&p);

    EQ(dcp_db_close(db), DCP_SUCCESS);
    fclose(fd);
}
