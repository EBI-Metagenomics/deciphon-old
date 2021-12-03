#include "standard_db_examples.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include "standard_db.h"

void std_db_examples_new_ex1(char const *filepath)
{
    imm_example1_init();

    struct imm_hmm *null = &imm_example1.null.hmm;
    struct imm_hmm *alt = &imm_example1.hmm;

    struct imm_example1 *m = &imm_example1;
    FILE *fd = fopen(filepath, "wb");
    NOTNULL(fd);
    struct dcp_standard_db db;
    dcp_standard_db_init(&db);
    EQ(dcp_standard_db_openw(&db, fd, &m->code), DONE);

    /* Profile 0 */
    struct standard_profile p;
    standard_profile_init(&p, &m->code);
    profile_nameit(&p.super, meta("NAME0", "ACC0"));
    EQ(imm_hmm_reset_dp(null, imm_super(&m->null.n), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(alt, imm_super(&m->end), &p.dp.alt), IMM_SUCCESS);
    EQ(dcp_standard_db_write(&db, &p), DONE);

    /* Profile 1 */
    struct imm_mute_state state;
    imm_mute_state_init(&state, 3, &m->abc);
    struct imm_hmm hmm;
    imm_hmm_init(&hmm, &m->code);
    EQ(imm_hmm_add_state(&hmm, imm_super(&state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(&hmm, imm_super(&state), imm_log(0.3)), IMM_SUCCESS);
    profile_nameit(&p.super, meta("NAME1", "ACC1"));
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.alt), IMM_SUCCESS);
    EQ(dcp_standard_db_write(&db, &p), DONE);

    standard_profile_del(&p);
    EQ(dcp_standard_db_close(&db), DONE);
    fclose(fd);
}

void std_db_examples_new_ex2(char const *filepath)
{
    imm_example2_init();

    struct imm_hmm *null = &imm_example2.null.hmm;
    struct imm_hmm *alt = &imm_example2.hmm;

    struct imm_example2 *m = &imm_example2;
    struct imm_abc const *abc = imm_super(imm_super(m->dna));
    struct imm_code const *code = &m->code;
    FILE *fd = fopen(filepath, "wb");
    NOTNULL(fd);
    struct dcp_standard_db db;
    dcp_standard_db_init(&db);
    EQ(dcp_standard_db_openw(&db, fd, code), DONE);

    /* Profile 0 */
    struct standard_profile p;
    standard_profile_init(&p, code);
    profile_nameit(&p.super, meta("NAME0", "ACC0"));
    EQ(imm_hmm_reset_dp(null, imm_super(&m->null.n), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(alt, imm_super(&m->end), &p.dp.alt), IMM_SUCCESS);
    EQ(dcp_standard_db_write(&db, &p), DONE);

    /* Profile 1 */
    profile_nameit(&p.super, meta("NAME1", "ACC1"));
    struct imm_mute_state state;
    imm_mute_state_init(&state, 3, abc);
    struct imm_hmm hmm;
    imm_hmm_init(&hmm, &m->code);
    EQ(imm_hmm_add_state(&hmm, imm_super(&state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(&hmm, imm_super(&state), imm_log(0.3)), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.alt), IMM_SUCCESS);
    EQ(dcp_standard_db_write(&db, &p), DONE);

    standard_profile_del(&p);
    EQ(dcp_standard_db_close(&db), DONE);
    fclose(fd);
}
