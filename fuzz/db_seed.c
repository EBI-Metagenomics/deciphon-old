#include "dcp/dcp.h"

static void write_example1(void);
static void write_example2(void);

int main(void)
{
    write_example1();
    write_example2();
    return 0;
}

static void write_example1(void)
{
    imm_example1_init();
    struct imm_example1 *m = &imm_example1;
    FILE *fd = fopen(CORPUS "/example1.dcp", "wb");
    struct dcp_db *db = dcp_db_openw(fd, &m->abc);

    /* Profile 0 */
    struct dcp_profile prof = {0};
    dcp_profile_init(&m->abc, &prof);
    prof.idx = 0;
    prof.mt = metadata("NAME0", "ACC0");
    imm_hmm_reset_dp(m->null.hmm, imm_super(m->null.n), prof.dp.null);
    imm_hmm_reset_dp(m->hmm, imm_super(m->end), prof.dp.alt);
    dcp_db_write(db, &prof);

    /* Profile 1 */
    struct imm_mute_state *state = imm_mute_state_new(3, &m->abc);
    struct imm_hmm *hmm = imm_hmm_new(&m->abc);
    imm_hmm_add_state(hmm, imm_super(state));
    imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3));
    prof.idx = 1;
    prof.mt = metadata("NAME1", "ACC1");
    imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.null);
    imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.alt);
    dcp_db_write(db, &prof);
    imm_del(hmm);
    imm_del(state);

    fclose(fd);
}

static void write_example2(void)
{
    imm_example2_init();
    struct imm_example2 *m = &imm_example2;
    struct imm_abc const *abc = imm_super(imm_super(m->dna));
    FILE *fd = fopen(CORPUS "/example2.dcp", "wb");
    struct dcp_db *db = dcp_db_openw(fd, abc);

    /* Profile 0 */
    struct dcp_profile prof = {0};
    dcp_profile_init(abc, &prof);
    prof.idx = 0;
    prof.mt = metadata("NAME0", "ACC0");
    imm_hmm_reset_dp(m->null.hmm, imm_super(m->null.n), prof.dp.null);
    imm_hmm_reset_dp(m->hmm, imm_super(m->end), prof.dp.alt);
    dcp_db_write(db, &prof);

    /* Profile 1 */
    struct imm_mute_state *state = imm_mute_state_new(3, abc);
    struct imm_hmm *hmm = imm_hmm_new(abc);
    imm_hmm_add_state(hmm, imm_super(state));
    imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3));
    prof.idx = 1;
    prof.mt = metadata("NAME1", "ACC1");
    imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.null);
    imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.alt);
    dcp_db_write(db, &prof);
    imm_del(hmm);
    imm_del(state);

    dcp_profile_deinit(&prof);
    dcp_db_close(db);
    imm_example2_deinit();
    fclose(fd);
}
