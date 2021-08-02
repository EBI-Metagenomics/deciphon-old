#include "cass/cass.h"
#include "dcp/dcp.h"
#include "static_tensor.h"
#include <stdlib.h>
#include <string.h>
/* #include "pfam24_results.h" */

void test_very_long_seq(void);

int main(void)
{
    test_very_long_seq();
    return cass_status();
}

#define logliks_shape(i) SHAPE(i, 24, 2)

static imm_float logliks[];

void test_very_long_seq(void)
{
    char const *filepath = PFAM24_FILEPATH;

    struct dcp_server *server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    struct dcp_task_cfg cfg = {true, true, true, false};
    struct dcp_task *task = dcp_task_create(cfg);

    unsigned rep = 10000;
    char const sep_str[] = "CAG";
    char const leader_thr[] =
        "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG";
    char *seq =
        malloc(strlen(leader_thr) * rep + strlen(sep_str) * (rep - 1) + 1);
    size_t k = 0;
    for (unsigned i = 0; i < rep; ++i)
    {
        if (i > 0)
        {
            for (unsigned j = 0; j < strlen(sep_str); ++j)
                seq[k++] = sep_str[j];
        }
        for (unsigned j = 0; j < strlen(leader_thr); ++j)
            seq[k++] = leader_thr[j];
    }
    seq[k] = '\0';

    dcp_task_add_seq(task, seq);
    dcp_server_add_task(server, task);

    while (!dcp_task_end(task))
    {
        struct dcp_result const *r = dcp_task_read(task);
        if (!r)
        {
            dcp_sleep(10);
            continue;
        }

        uint32_t profid = dcp_result_profid(r);
        for (unsigned j = 0; j < 2; ++j)
        {
            enum dcp_model m = dcp_models[j];
            cass_close(dcp_result_loglik(r, m), T(logliks, profid, j));
        }
        dcp_server_free_result(server, r);
    }
    dcp_server_free_task(server, task);

    dcp_server_stop(server);
    dcp_server_join(server);

    cass_equal(dcp_server_destroy(server), 0);

    free(seq);
}

#define ENTRY(i0, i1, V) [_(logliks, i0, i1)] = V
static imm_float logliks[] = {
    ENTRY(17, 0, -878184.966262600501), ENTRY(17, 1, -1023714.276343169273),
    ENTRY(11, 0, -870332.288492417196), ENTRY(11, 1, -1023714.276343169273),
    ENTRY(7, 0, -862310.171913801576),  ENTRY(7, 1, -1023714.276343169273),
    ENTRY(15, 0, -878177.436623534537), ENTRY(15, 1, -1023714.276343169273),
    ENTRY(10, 0, -878184.461306311539), ENTRY(10, 1, -1023714.276343169273),
    ENTRY(6, 0, -878183.011237323401),  ENTRY(6, 1, -1023714.276343169273),
    ENTRY(22, 0, -878182.975882522529), ENTRY(22, 1, -1023714.276343169273),
    ENTRY(19, 0, -836852.343655026285), ENTRY(19, 1, -1023714.276343169273),
    ENTRY(3, 0, -846605.119309381582),  ENTRY(3, 1, -1023714.276343169273),
    ENTRY(12, 0, -853514.197142719175), ENTRY(12, 1, -1023714.276343169273),
    ENTRY(1, 0, -878183.910812326823),  ENTRY(1, 1, -1023714.276343169273),
    ENTRY(20, 0, -864539.008297724649), ENTRY(20, 1, -1023714.276343169273),
    ENTRY(13, 0, -843319.466108072898), ENTRY(13, 1, -1023714.276343169273),
    ENTRY(4, 0, -870823.018622492440),  ENTRY(4, 1, -1023714.276343169273),
    ENTRY(21, 0, -851186.596800211933), ENTRY(21, 1, -1023714.276343169273),
    ENTRY(18, 0, -878183.015136126778), ENTRY(18, 1, -1023714.276343169273),
    ENTRY(14, 0, -866997.990194887505), ENTRY(14, 1, -1023714.276343169273),
    ENTRY(8, 0, -867146.648247761186),  ENTRY(8, 1, -1023714.276343169273),
    ENTRY(0, 0, -779988.430003659800),  ENTRY(0, 1, -1023714.276343169273),
    ENTRY(23, 0, -872856.962155637797), ENTRY(23, 1, -1023714.276343169273),
    ENTRY(9, 0, -855354.364892750862),  ENTRY(9, 1, -1023714.276343169273),
    ENTRY(5, 0, -878182.609842869337),  ENTRY(5, 1, -1023714.276343169273),
    ENTRY(16, 0, -878184.424944799626), ENTRY(16, 1, -1023714.276343169273),
    ENTRY(2, 0, -849636.960957933916),  ENTRY(2, 1, -1023714.276343169273),
};
#undef ENTRY
