#include "standard_db.h"
#include "hope/hope.h"
#include "standard_db_examples.h"
#include "standard_profile.h"

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
    test_db_openr_example1();
    test_db_openw_example2();
    test_db_openr_example2();
    return hope_status();
}

void test_db_openw_empty(void)
{
    struct imm_dna const *dna = &imm_dna_iupac;
    struct imm_abc const *abc = imm_super(imm_super(dna));
    struct imm_code code;
    imm_code_init(&code, abc);
    FILE *fd = fopen(TMPDIR "/empty.dcp", "wb");
    NOTNULL(fd);
    struct dcp_standard_db db;
    dcp_standard_db_init(&db);
    EQ(dcp_standard_db_openw(&db, fd, &code), DONE);
    EQ(dcp_standard_db_close(&db), DONE);
    fclose(fd);
}

void test_db_openr_empty(void)
{
    FILE *fd = fopen(TMPDIR "/empty.dcp", "rb");
    NOTNULL(fd);
    struct dcp_standard_db db;
    dcp_standard_db_init(&db);
    EQ(dcp_standard_db_openr(&db, fd), DONE);
    EQ(dcp_db_float_size(&db.super), IMM_FLOAT_BYTES);
    EQ(dcp_db_prof_typeid(&db.super), DCP_STANDARD_PROFILE);
    struct imm_abc const *abc = dcp_standard_db_abc(&db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    struct imm_dna const *dna = (struct imm_dna *)abc;
    EQ(imm_abc_typeid(imm_super(imm_super(dna))), IMM_DNA);
    EQ(dcp_standard_db_close(&db), DONE);
    fclose(fd);
}

void test_db_openw_one_mute(void)
{
    struct imm_dna const *dna = &imm_dna_iupac;
    struct imm_abc const *abc = imm_super(imm_super(dna));
    struct imm_code code;
    imm_code_init(&code, abc);

    struct imm_mute_state state;
    imm_mute_state_init(&state, 3, abc);
    struct imm_hmm hmm;
    imm_hmm_init(&hmm, &code);
    EQ(imm_hmm_add_state(&hmm, imm_super(&state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(&hmm, imm_super(&state), imm_log(0.3)), IMM_SUCCESS);

    FILE *fd = fopen(TMPDIR "/one_mute.dcp", "wb");
    NOTNULL(fd);
    struct dcp_standard_db db;
    dcp_standard_db_init(&db);
    EQ(dcp_standard_db_openw(&db, fd, &code), DONE);

    struct standard_profile p;
    standard_profile_init(&p, &code);
    prof_nameit(&p.super, meta("NAME0", "ACC0"));
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.alt), IMM_SUCCESS);
    EQ(dcp_standard_db_write(&db, &p), DONE);

    standard_profile_del(&p);
    EQ(dcp_standard_db_close(&db), DONE);
    fclose(fd);
}

void test_db_openr_one_mute(void)
{
    FILE *fd = fopen(TMPDIR "/one_mute.dcp", "rb");
    NOTNULL(fd);
    struct dcp_standard_db db;
    dcp_standard_db_init(&db);
    EQ(dcp_standard_db_openr(&db, fd), DONE);
    EQ(dcp_db_float_size(&db.super), IMM_FLOAT_BYTES);
    EQ(dcp_db_prof_typeid(&db.super), DCP_STANDARD_PROFILE);
    struct imm_abc const *abc = dcp_standard_db_abc(&db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    struct imm_dna const *dna = (struct imm_dna *)abc;
    EQ(imm_abc_typeid(imm_super(imm_super(dna))), IMM_DNA);

    EQ(dcp_db_nprofiles(&db.super), 1);

    struct meta mt = dcp_db_meta(&db.super, 0);

    EQ(mt.name, "NAME0");
    EQ(mt.acc, "ACC0");

    EQ(dcp_standard_db_close(&db), DONE);
    fclose(fd);
}

void test_db_openw_example1(void)
{
    std_db_examples_new_ex1(TMPDIR "/example1.dcp");
}

void test_db_openr_example1(void)
{
    FILE *fd = fopen(TMPDIR "/example1.dcp", "rb");
    NOTNULL(fd);
    struct dcp_standard_db db;
    dcp_standard_db_init(&db);
    EQ(dcp_standard_db_openr(&db, fd), DONE);
    EQ(dcp_db_float_size(&db.super), IMM_FLOAT_BYTES);
    EQ(dcp_db_prof_typeid(&db.super), DCP_STANDARD_PROFILE);
    EQ(imm_abc_typeid(dcp_standard_db_abc(&db)), IMM_ABC);

    EQ(dcp_db_nprofiles(&db.super), 2);

    struct meta mt[2] = {dcp_db_meta(&db.super, 0), dcp_db_meta(&db.super, 1)};
    EQ(mt[0].name, "NAME0");
    EQ(mt[0].acc, "ACC0");
    EQ(mt[1].name, "NAME1");
    EQ(mt[1].acc, "ACC1");

    unsigned nprofs = 0;
    struct imm_prod prod = imm_prod();
    struct standard_profile *p = dcp_standard_db_profile(&db);
    while (!dcp_db_end(&db.super))
    {
        EQ(dcp_standard_db_read(&db, p), DONE);
        EQ(dcp_prof_typeid(&p->super), DCP_STANDARD_PROFILE);
        if (p->super.idx == 0)
        {
            struct imm_task *task = imm_task_new(&p->dp.alt);
            struct imm_abc const *abc = dcp_standard_db_abc(&db);
            struct imm_seq seq = imm_seq(imm_str(imm_example1_seq), abc);
            EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
            EQ(imm_dp_viterbi(&p->dp.alt, task, &prod), IMM_SUCCESS);
            CLOSE(prod.loglik, -65826.0106185297);
            imm_del(task);
        }
        ++nprofs;
    }
    EQ(nprofs, 2);

    imm_del(&prod);
    EQ(dcp_standard_db_close(&db), DONE);
    fclose(fd);
}

void test_db_openw_example2(void)
{
    std_db_examples_new_ex2(TMPDIR "/example2.dcp");
}

void test_db_openr_example2(void)
{
    FILE *fd = fopen(TMPDIR "/example2.dcp", "rb");
    NOTNULL(fd);
    struct dcp_standard_db db;
    dcp_standard_db_init(&db);
    EQ(dcp_standard_db_openr(&db, fd), DONE);
    EQ(dcp_db_float_size(&db.super), IMM_FLOAT_BYTES);
    EQ(dcp_db_prof_typeid(&db.super), DCP_STANDARD_PROFILE);
    struct imm_abc const *abc = dcp_standard_db_abc(&db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(dcp_db_nprofiles(&db.super), 2);

    struct meta mt[2] = {dcp_db_meta(&db.super, 0), dcp_db_meta(&db.super, 1)};
    EQ(mt[0].name, "NAME0");
    EQ(mt[0].acc, "ACC0");
    EQ(mt[1].name, "NAME1");
    EQ(mt[1].acc, "ACC1");

    unsigned nprofs = 0;
    struct imm_prod prod = imm_prod();
    struct standard_profile *p = dcp_standard_db_profile(&db);
    while (!dcp_db_end(&db.super))
    {
        EQ(dcp_standard_db_read(&db, p), DONE);
        EQ(dcp_prof_typeid(&p->super), DCP_STANDARD_PROFILE);
        if (p->super.idx == 0)
        {
            struct imm_task *task = imm_task_new(&p->dp.alt);
            struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
            EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
            EQ(imm_dp_viterbi(&p->dp.alt, task, &prod), IMM_SUCCESS);
            CLOSE(prod.loglik, -1622.8488101101);
            imm_del(task);
        }
        ++nprofs;
    }
    EQ(nprofs, 2);

    imm_del(&prod);
    EQ(dcp_standard_db_close(&db), DONE);
    fclose(fd);
}
