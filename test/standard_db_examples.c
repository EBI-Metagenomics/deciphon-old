#include "standard_db_examples.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include "standard_db.h"

void standard_db_examples_new_ex1(char const *filepath)
{
    imm_example1_init();

    struct imm_hmm *null = &imm_example1.null.hmm;
    struct imm_hmm *alt = &imm_example1.hmm;

    struct imm_example1 *m = &imm_example1;
    FILE *fd = fopen(filepath, "wb");
    NOTNULL(fd);
    struct standard_db db;
    standard_db_init(&db);
    EQ(standard_db_openw(&db, fd, &m->code), RC_DONE);

    /* Profile 0 */
    struct standard_profile p;
    standard_profile_init(&p, &m->code);
    profile_nameit(&p.super, metadata("NAME0", "ACC0"));
    EQ(imm_hmm_reset_dp(null, imm_super(&m->null.n), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(alt, imm_super(&m->end), &p.dp.alt), IMM_SUCCESS);
    EQ(standard_db_write(&db, &p), RC_DONE);

    /* Profile 1 */
    struct imm_mute_state state;
    imm_mute_state_init(&state, 3, &m->abc);
    struct imm_hmm hmm;
    imm_hmm_init(&hmm, &m->code);
    EQ(imm_hmm_add_state(&hmm, imm_super(&state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(&hmm, imm_super(&state), imm_log(0.3)), IMM_SUCCESS);
    profile_nameit(&p.super, metadata("NAME1", "ACC1"));
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.alt), IMM_SUCCESS);
    EQ(standard_db_write(&db, &p), RC_DONE);

    standard_profile_del(&p);
    EQ(standard_db_close(&db), RC_DONE);
    fclose(fd);
}

void standard_db_examples_new_ex2(char const *filepath)
{
    imm_example2_init();

    struct imm_hmm *null = &imm_example2.null.hmm;
    struct imm_hmm *alt = &imm_example2.hmm;

    struct imm_example2 *m = &imm_example2;
    struct imm_abc const *abc = imm_super(imm_super(m->dna));
    struct imm_code const *code = &m->code;
    FILE *fd = fopen(filepath, "wb");
    NOTNULL(fd);
    struct standard_db db;
    standard_db_init(&db);
    EQ(standard_db_openw(&db, fd, code), RC_DONE);

    /* Profile 0 */
    struct standard_profile p;
    standard_profile_init(&p, code);
    profile_nameit(&p.super, metadata("NAME0", "ACC0"));
    EQ(imm_hmm_reset_dp(null, imm_super(&m->null.n), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(alt, imm_super(&m->end), &p.dp.alt), IMM_SUCCESS);
    EQ(standard_db_write(&db, &p), RC_DONE);

    /* Profile 1 */
    profile_nameit(&p.super, metadata("NAME1", "ACC1"));
    struct imm_mute_state state;
    imm_mute_state_init(&state, 3, abc);
    struct imm_hmm hmm;
    imm_hmm_init(&hmm, &m->code);
    EQ(imm_hmm_add_state(&hmm, imm_super(&state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(&hmm, imm_super(&state), imm_log(0.3)), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(&hmm, imm_super(&state), &p.dp.alt), IMM_SUCCESS);
    EQ(standard_db_write(&db, &p), RC_DONE);

    standard_profile_del(&p);
    EQ(standard_db_close(&db), RC_DONE);
    fclose(fd);
}
