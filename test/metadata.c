#include "cass/cass.h"
#include "dcp/dcp.h"

static void test_metadata(void);

int main(void)
{
    test_metadata();
    return cass_status();
}

static char const* accs[] = {
    "PF08254.12", "PF01797.17", "PF00742.20", "PF00696.29", "PF03447.17", "PF13840.7",  "PF01842.26", "PF00288.27",
    "PF08544.14", "PF00291.26", "PF14821.7",  "PF10697.10", "PF00923.20", "PF00994.25", "PF10417.10", "PF12574.9",
    "PF09847.10", "PF00244.21", "PF16998.6",  "PF00389.31", "PF02826.20", "PF00198.24", "PF16078.6",  "PF04029.15",
};

static char const* names[] = {
    "Leader_Thr",   "Y1_Tnp",         "Homoserine_dh",  "AA_kinase",    "NAD_binding_3",   "ACT_7",
    "ACT",          "GHMP_kinases_N", "GHMP_kinases_C", "PALP",         "Thr_synth_N",     "DUF2502",
    "TAL_FSA",      "MoCF_biosynth",  "1-cysPrx_C",     "120_Rick_ant", "12TM_1",          "14-3-3",
    "17kDa_Anti_2", "2-Hacid_dh",     "2-Hacid_dh_C",   "2-oxoacid_dh", "2-oxogl_dehyd_N", "2-ph_phosp",
};

static void test_metadata(void)
{
    char const* filepath = PFAM24_FILEPATH;

    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_nprofiles(server), 24);
    for (unsigned i = 0; i < 24; ++i) {
        struct dcp_metadata const* mt = dcp_server_metadata(server, i);
        cass_equal(strcmp(dcp_metadata_acc(mt), accs[i]), 0);
        cass_equal(strcmp(dcp_metadata_name(mt), names[i]), 0);
    }

    cass_equal(dcp_server_destroy(server), 0);
}
