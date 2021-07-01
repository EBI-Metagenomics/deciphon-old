#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_db_openw_empty(void);
void test_db_openr_empty(void);
void test_db_openw_one_mute(void);
void test_db_openr_one_mute(void);
void test_db_openw_example1(void);
void test_db_openr_example1(void);
void test_db_openw_example2(void);
void test_db_openr_example2(void);

int main(void)
{
    test_db_openw_empty();
    test_db_openr_empty();
    test_db_openw_one_mute();
    test_db_openr_one_mute();
    test_db_openw_example1();
    /* test_db_openr_example1(); */
    /* test_db_openw_example2(); */
    /* test_db_openr_example2(); */
    return hope_status();
}

void test_db_openw_empty(void)
{
    struct imm_dna const *dna = &imm_dna_default;
    struct imm_abc const *abc = imm_super(imm_super(dna));
    FILE *fd = fopen(TMPDIR "/empty.dcp", "wb");
    struct dcp_db *db = dcp_db_openw(fd, abc);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
}

void test_db_openr_empty(void)
{
    FILE *fd = fopen(TMPDIR "/empty.dcp", "rb");
    struct dcp_db *db = dcp_db_openr(fd);
    NOTNULL(db);
    struct imm_abc const *abc = dcp_db_abc(db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    struct imm_dna const *dna = (struct imm_dna *)abc;
    EQ(imm_abc_typeid(imm_super(imm_super(dna))), IMM_DNA);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
}

void test_db_openw_one_mute(void)
{
    struct imm_dna const *dna = &imm_dna_default;
    struct imm_abc const *abc = imm_super(imm_super(dna));

    struct imm_mute_state *state = imm_mute_state_new(3, abc);
    struct imm_hmm *hmm = imm_hmm_new(abc);
    EQ(imm_hmm_add_state(hmm, imm_super(state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3)), IMM_SUCCESS);

    FILE *fd = fopen(TMPDIR "/one_mute.dcp", "wb");
    struct dcp_db *db = dcp_db_openw(fd, abc);

    struct dcp_profile prof = {0};
    dcp_profile_init(abc, &prof);
    prof.idx = 0;
    prof.mt = dcp_metadata("NAME0", "ACC0");
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.alt), IMM_SUCCESS);
    EQ(dcp_db_write(db, &prof), IMM_SUCCESS);

    imm_del(hmm);
    imm_del(state);
    dcp_profile_deinit(&prof);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
}

void test_db_openr_one_mute(void)
{
    FILE *fd = fopen(TMPDIR "/one_mute.dcp", "rb");
    struct dcp_db *db = dcp_db_openr(fd);
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
    fclose(fd);
}

void test_db_openw_example1(void)
{
    imm_example1_init();
    struct imm_example1 *m = &imm_example1;
    FILE *fd = fopen(TMPDIR "/example1.dcp", "wb");
    struct dcp_db *db = dcp_db_openw(fd, &m->abc);

    /* Profile 0 */
    struct dcp_profile prof = {0};
    dcp_profile_init(&m->abc, &prof);
    prof.idx = 0;
    prof.mt = dcp_metadata("NAME0", "ACC0");
    EQ(imm_hmm_reset_dp(m->null.hmm, imm_super(m->null.n), prof.dp.null),
       IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(m->hmm, imm_super(m->end), prof.dp.alt), IMM_SUCCESS);
    EQ(dcp_db_write(db, &prof), IMM_SUCCESS);

    /* Profile 1 */
#if 0
    struct imm_mute_state *state = imm_mute_state_new(3, &m->abc);
    struct imm_hmm *hmm = imm_hmm_new(&m->abc);
    EQ(imm_hmm_add_state(hmm, imm_super(state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3)), IMM_SUCCESS);
    prof.idx = 1;
    prof.mt = dcp_metadata("NAME1", "ACC1");
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.alt), IMM_SUCCESS);
    EQ(dcp_db_write(db, &prof), IMM_SUCCESS);
    imm_del(hmm);
    imm_del(state);
#endif

    dcp_profile_deinit(&prof);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    imm_example1_deinit();
    fclose(fd);
}

void test_db_openr_example1(void)
{
    FILE *fd = fopen(TMPDIR "/example1.dcp", "rb");
    struct dcp_db *db = dcp_db_openr(fd);
    NOTNULL(db);
    struct imm_abc const *abc = dcp_db_abc(db);
    EQ(imm_abc_typeid(abc), IMM_ABC);

    EQ(dcp_db_nprofiles(db), 2);

    struct dcp_metadata mt[2] = {dcp_db_metadata(db, 0),
                                 dcp_db_metadata(db, 1)};
    EQ(mt[0].name, "NAME0");
    EQ(mt[0].acc, "ACC0");
    EQ(mt[1].name, "NAME1");
    EQ(mt[1].acc, "ACC1");

    unsigned nprofs = 0;
    struct dcp_profile prof = {0};
    dcp_profile_init(abc, &prof);
    struct imm_result result = imm_result();
    while (!dcp_db_end(db))
    {
        EQ(dcp_db_read(db, &prof), IMM_SUCCESS);
        if (prof.idx == 0)
        {
            struct imm_task *task = imm_task_new(prof.dp.alt);
            struct imm_seq seq = imm_seq(imm_str(imm_example1_seq), prof.abc);
            EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
            EQ(imm_dp_viterbi(prof.dp.alt, task, &result), IMM_SUCCESS);
            CLOSE(result.loglik, -65826.0106185297);
            imm_del(task);
        }
        ++nprofs;
    }
    EQ(nprofs, 2);

    imm_del(&result);
    dcp_profile_deinit(&prof);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
}

void test_db_openw_example2(void)
{
    imm_example2_init();
    struct imm_example2 *m = &imm_example2;
    struct imm_abc const *abc = imm_super(imm_super(m->dna));
    FILE *fd = fopen(TMPDIR "/example2.dcp", "wb");
    struct dcp_db *db = dcp_db_openw(fd, abc);

    /* Profile 0 */
    struct dcp_profile prof = {0};
    dcp_profile_init(abc, &prof);
    prof.idx = 0;
    prof.mt = dcp_metadata("NAME0", "ACC0");
    EQ(imm_hmm_reset_dp(m->null.hmm, imm_super(m->null.n), prof.dp.null),
       IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(m->hmm, imm_super(m->end), prof.dp.alt), IMM_SUCCESS);
    EQ(dcp_db_write(db, &prof), IMM_SUCCESS);

    /* Profile 1 */
    struct imm_mute_state *state = imm_mute_state_new(3, abc);
    struct imm_hmm *hmm = imm_hmm_new(abc);
    EQ(imm_hmm_add_state(hmm, imm_super(state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3)), IMM_SUCCESS);
    prof.idx = 1;
    prof.mt = dcp_metadata("NAME1", "ACC1");
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.alt), IMM_SUCCESS);
    EQ(dcp_db_write(db, &prof), IMM_SUCCESS);
    imm_del(hmm);
    imm_del(state);

    dcp_profile_deinit(&prof);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    imm_example2_deinit();
    fclose(fd);
}

void test_db_openr_example2(void)
{
    FILE *fd = fopen(TMPDIR "/example2.dcp", "rb");
    struct dcp_db *db = dcp_db_openr(fd);
    NOTNULL(db);
    struct imm_abc const *abc = dcp_db_abc(db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(dcp_db_nprofiles(db), 2);

    struct dcp_metadata mt[2] = {dcp_db_metadata(db, 0),
                                 dcp_db_metadata(db, 1)};
    EQ(mt[0].name, "NAME0");
    EQ(mt[0].acc, "ACC0");
    EQ(mt[1].name, "NAME1");
    EQ(mt[1].acc, "ACC1");

    unsigned nprofs = 0;
    struct dcp_profile prof = {0};
    dcp_profile_init(abc, &prof);
    struct imm_result result = imm_result();
    while (!dcp_db_end(db))
    {
        EQ(dcp_db_read(db, &prof), IMM_SUCCESS);
        if (prof.idx == 0)
        {
            struct imm_task *task = imm_task_new(prof.dp.alt);
            struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), prof.abc);
            EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
            EQ(imm_dp_viterbi(prof.dp.alt, task, &result), IMM_SUCCESS);
            CLOSE(result.loglik, -1622.8488101101);
            imm_del(task);
        }
        ++nprofs;
    }
    EQ(nprofs, 2);

    imm_del(&result);
    dcp_profile_deinit(&prof);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
}
