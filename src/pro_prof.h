#ifndef PRO_PROF_H
#define PRO_PROF_H

#include "dcp/rc.h"
#include <stdio.h>

struct cmp_ctx_s;
struct dcp_pro_prof;

enum dcp_rc pro_prof_read(struct dcp_pro_prof *prof, struct cmp_ctx_s *ctx);
enum dcp_rc pro_prof_write(struct dcp_pro_prof const *prof,
                           struct cmp_ctx_s *ctx);

#endif
