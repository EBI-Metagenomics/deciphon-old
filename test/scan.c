#include "fs.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "unused.h"
#include "work.h"

// download minifam.dcp
// download minifam.hmm
// download consensus.json
// download prods_file_20221021.tsv

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    COND(!global_run_once());

    work_init();
    // int rc = work_run(seqs, db, prod, true, false);

    return hope_status();
}
