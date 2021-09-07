#ifndef STD_PROF_H
#define STD_PROF_H

#include "dcp/rc.h"
#include <stdio.h>

struct dcp_std_prof;

enum dcp_rc std_prof_read(struct dcp_std_prof *prof, FILE *restrict fd);
enum dcp_rc std_prof_write(struct dcp_std_prof const *prof, FILE *restrict fd);

#endif