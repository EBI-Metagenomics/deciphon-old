#include "scan/scan.h"
#include "fs.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "unused.h"
#include "work.h"

// download prods_file_20221021.tsv

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);

    struct scan_cfg cfg = {1, 10., true, false};
    scan_init(cfg);

    EQ(global_run(), 0);
    EQ(scan_setup("minifam.dcp", "consensus.json"), 0);
    EQ(global_run(), 0);
    EQ(global_run(), 0);
    EQ(global_run(), 0);

    // work_init();
    // COND(!work_run("consensus.json", "minifam.dcp", "prods.tsv", true,
    // false));

    // enum state state = work_state();
    // fprintf(stderr, "State: %d\n", state);

    // global_run();
    // state = work_state();
    // fprintf(stderr, "State: %d\n", state);

    return hope_status();
}
