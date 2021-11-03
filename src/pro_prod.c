#include "pro_prod.h"
#include "prod.h"
#include <cco/iter.h>

enum dcp_rc pro_prod_write(struct pro_prod *p, FILE *restrict fd)
{
    enum dcp_rc rc = prod_write(&p->super, fd);
    if (rc) return rc;

    struct cco_iter iter = {0};
    struct pro_match *match = NULL;
    if (!(match = cco_iter_next_entry(&iter, match, node))) return rc;

    goto enter;
    for (; match; match = cco_iter_next_entry(&iter, match, node))
    {
        if ((rc = pro_match_write_sep(fd))) return rc;
    enter:
        if ((rc = pro_match_write(match, fd))) return rc;
    }
    return rc;
}
