#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_db_openw_empty(void);
void test_db_openr_empty(void);
void test_db_openw_one_mute(void);
void test_db_openr_one_mute(void);
void test_db_openw_example1(void);
void test_db_openr_example1(void);

int main(void)
{
    test_db_openw_empty();
    test_db_openr_empty();
    test_db_openw_one_mute();
    test_db_openr_one_mute();
    test_db_openw_example1();
    test_db_openr_example1();
    return hope_status();
}

void test_db_openw_empty(void)
{
    struct imm_dna const *dna = &imm_dna_default;
    struct imm_abc const *abc = imm_super(imm_super(dna));
    struct dcp_db *db = dcp_db_openw(TMPDIR "/empty.dcp", abc);
    EQ(dcp_db_close(db), IMM_SUCCESS);
}

void test_db_openr_empty(void)
{
    struct dcp_db *db = dcp_db_openr(TMPDIR "/empty.dcp");
    NOTNULL(db);
    struct imm_abc const *abc = dcp_db_abc(db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    struct imm_dna const *dna = (struct imm_dna *)abc;
    EQ(imm_abc_typeid(imm_super(imm_super(dna))), IMM_DNA);
    EQ(dcp_db_close(db), IMM_SUCCESS);
}

void test_db_openw_one_mute(void)
{
    struct imm_dna const *dna = &imm_dna_default;
    struct imm_abc const *abc = imm_super(imm_super(dna));

    struct imm_mute_state *state = imm_mute_state_new(3, abc);
    struct imm_hmm *hmm = imm_hmm_new(abc);
    EQ(imm_hmm_add_state(hmm, imm_super(state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3)), IMM_SUCCESS);

    struct dcp_db *db = dcp_db_openw(TMPDIR "/one_mute.dcp", abc);

    struct dcp_profile prof = dcp_profile(abc);
    prof.idx = 0;
    prof.mt = dcp_metadata("NAME0", "ACC0");
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.alt), IMM_SUCCESS);
    EQ(dcp_db_write(db, &prof), IMM_SUCCESS);

    imm_del(prof.dp.null);
    imm_del(prof.dp.alt);
    imm_del(hmm);
    imm_del(state);
    EQ(dcp_db_close(db), IMM_SUCCESS);
}

void test_db_openr_one_mute(void)
{
    struct dcp_db *db = dcp_db_openr(TMPDIR "/one_mute.dcp");
    NOTNULL(db);
    struct imm_abc const *abc = dcp_db_abc(db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    struct imm_dna const *dna = (struct imm_dna *)abc;
    EQ(imm_abc_typeid(imm_super(imm_super(dna))), IMM_DNA);

    EQ(dcp_db_nprofiles(db), 1);

    struct dcp_metadata mt = dcp_db_metadata(db, 0);

    EQ(mt.name, "NAME0");
    EQ(mt.acc, "ACC0");

    EQ(dcp_db_close(db), IMM_SUCCESS);
}

void test_db_openw_example1(void)
{
    imm_example1_init();
    struct imm_example1 *m1 = &imm_example1;
    struct dcp_db *db = dcp_db_openw(TMPDIR "/example1.dcp", &m1->abc);

    /* Profile 0 */
    struct dcp_profile prof = dcp_profile(&m1->abc);
    prof.idx = 0;
    prof.mt = dcp_metadata("NAME0", "ACC0");
    EQ(imm_hmm_reset_dp(m1->null.hmm, imm_super(m1->null.n), prof.dp.null),
       IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(m1->hmm, imm_super(m1->end), prof.dp.alt), IMM_SUCCESS);
    EQ(dcp_db_write(db, &prof), IMM_SUCCESS);

    /* Profile 1 */
    struct imm_mute_state *state = imm_mute_state_new(3, &m1->abc);
    struct imm_hmm *hmm = imm_hmm_new(&m1->abc);
    EQ(imm_hmm_add_state(hmm, imm_super(state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3)), IMM_SUCCESS);
    prof.idx = 1;
    prof.mt = dcp_metadata("NAME1", "ACC1");
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.alt), IMM_SUCCESS);
    EQ(dcp_db_write(db, &prof), IMM_SUCCESS);
    imm_del(hmm);
    imm_del(state);

    imm_del(prof.dp.null);
    imm_del(prof.dp.alt);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    imm_example1_deinit();
}

void test_db_openr_example1(void)
{
    struct dcp_db *db = dcp_db_openr(TMPDIR "/example1.dcp");
    NOTNULL(db);
    struct imm_abc const *abc = dcp_db_abc(db);
    EQ(imm_abc_typeid(abc), IMM_ABC);

    EQ(dcp_db_nprofiles(db), 2);

    struct dcp_metadata mt[2] = {dcp_db_metadata(db, 0), dcp_db_metadata(db, 1)};
    EQ(mt[0].name, "NAME0");
    EQ(mt[0].acc, "ACC0");
    EQ(mt[1].name, "NAME1");
    EQ(mt[1].acc, "ACC1");

    EQ(dcp_db_close(db), IMM_SUCCESS);
}