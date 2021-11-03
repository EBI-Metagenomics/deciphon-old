#ifndef PRO_PROD_H
#define PRO_PROD_H

#include "pro_match.h"
#include "prod.h"

struct pro_prod
{
    struct prod super;
    struct cco_stack matches;
};

static inline void pro_prod_init(struct pro_prod *prod)
{
    cco_stack_init(&prod->matches);
}

static inline void pro_prod_add_match(struct pro_prod *prod,
                                      struct pro_match *match)
{
    cco_stack_put(&prod->matches, &match->node);
}

enum dcp_rc pro_prod_write(struct pro_prod *p, FILE *restrict fd);

#endif
