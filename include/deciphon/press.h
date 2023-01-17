#ifndef DECIPHON_PRESS_H
#define DECIPHON_PRESS_H

#include "deciphon/export.h"

struct dcp_press;

DCP_API struct dcp_press *dcp_press_new(void);
DCP_API int dcp_press_open(struct dcp_press *, char const *hmm, char const *db);
DCP_API long dcp_press_nsteps(struct dcp_press const *);
DCP_API int dcp_press_step(struct dcp_press *);
DCP_API int dcp_press_close(struct dcp_press *, int succesfully);
DCP_API void dcp_press_del(struct dcp_press const *);

#endif
