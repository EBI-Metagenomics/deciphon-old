#ifndef PRO_PROF_H
#define PRO_PROF_H

#include "dcp/rc.h"
#include <stdio.h>

struct dcp_pro_prof;

enum dcp_rc pro_prof_read(struct dcp_pro_prof *prof, FILE *restrict fd);
enum dcp_rc pro_prof_write(struct dcp_pro_prof const *prof, FILE *restrict fd);

#endif
