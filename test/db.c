#include "dcp/dcp.h"
#include "dcp/model.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include <imm/abc_types.h>
#include <imm/hmm.h>
#include <imm/log.h>

void test_db_openw_empty(void);
void test_db_openr_empty(void);
void test_db_openw_one_mute(void);

int main(void)
{
    test_db_openw_empty();
    test_db_openr_empty();
    test_db_openw_one_mute();
    return hope_status();
}

void test_db_openw_empty(void)
{
    struct imm_dna const *dna = &imm_dna_default;
    struct imm_abc const *abc = imm_super(imm_super(dna));
    struct dcp_db *db = dcp_db_openw(TMPDIR "/empty.dcp", abc);
    dcp_db_close(db);
}

void test_db_openr_empty(void)
{
    struct dcp_db *db = dcp_db_openr(TMPDIR "/empty.dcp");
    NOTNULL(db);
    struct imm_abc const *abc = dcp_db_abc(db);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    struct imm_dna const *dna = (struct imm_dna *)dna;
    EQ(imm_abc_typeid(imm_super(imm_super(dna))), IMM_DNA);
    dcp_db_close(db);
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

    struct dcp_profile prof;
    dcp_profile_init(&prof, abc);
    prof.idx = 0;
    prof.mt.acc = "ACC0";
    prof.mt.name = "NAME0";
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.null), IMM_SUCCESS);
    EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.alt), IMM_SUCCESS);
    dcp_db_write(db, &prof);

    dcp_db_close(db);
    imm_del(prof.null);
    imm_del(prof.alt);
    imm_del(hmm);
    imm_del(state);
}
