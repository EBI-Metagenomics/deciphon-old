#include "fs.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "unused.h"
#include "work.h"

// download prods_file_20221021.tsv

int main(int argc, char *argv[])
{
    return hope_status();
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    COND(!global_run_once());

    work_init();
    COND(!work_run("consensus.json", "minifam.dcp", "prods.tsv", true, false));

    global_run_once();
    enum state state = work_state();
    fprintf(stderr, "State: %d\n", state);

    global_run_once();
    state = work_state();
    fprintf(stderr, "State: %d\n", state);

    return hope_status();
}
