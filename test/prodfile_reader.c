#include "scan/prodfile_reader.h"
#include "hope.h"
#include "loop/global.h"
#include "rc.h"
#include "strhash.h"
#include <stddef.h>

int main(void)
{
    struct prod prod = {0};
    struct prodfile_reader *reader = NULL;

    eq(prodfile_reader_new(&reader), 0);
    eq(prodfile_reader_open(reader, TESTDIR "/prods_file_20221021.tsv"), 0);

    eq(prodfile_reader_next(reader, &prod), 0);
    eq(prod.scan_id, 1);
    eq(prod.seq_id, 1);
    eq(prod.profile_name, "PF00742.20");
    eq(prod.abc_name, "dna");
    close(prod.alt_loglik, -547.87713623046875);
    close(prod.null_loglik, -690.86773681640625);
    eq(prod.profile_typeid, "protein");
    eq(prod.version, "1.0.0");
    eq(strhash(prod.match), 25903);

    eq(prodfile_reader_next(reader, &prod), 0);
    eq(prod.scan_id, 1);
    eq(prod.seq_id, 2);
    eq(prod.profile_name, "PF00696.29");
    eq(prod.abc_name, "dna");
    close(prod.alt_loglik, -802.65130615234);
    close(prod.null_loglik, -974.47515869141);
    eq(prod.profile_typeid, "protein");
    eq(prod.version, "1.0.0");
    eq(strhash(prod.match), 20429);

    eq(prodfile_reader_next(reader, &prod), 0);
    eq(prod.scan_id, 1);
    eq(prod.seq_id, 3);
    eq(prod.profile_name, "PF16620.6");
    eq(prod.abc_name, "dna");
    close(prod.alt_loglik, -489.93957519531);
    close(prod.null_loglik, -667.29437255859);
    eq(prod.profile_typeid, "protein");
    eq(prod.version, "1.0.0");
    eq(strhash(prod.match), 39610);

    eq(prodfile_reader_next(reader, &prod), RC_END);

    eq(prodfile_reader_close(reader), 0);
    prodfile_reader_del(reader);
}
