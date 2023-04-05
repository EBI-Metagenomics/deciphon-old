#ifndef DECIPHON_PRESS_H
#define DECIPHON_PRESS_H

#include "deciphon/export.h"
#include <stdbool.h>

struct dcp_press;

DCP_API struct dcp_press *dcp_press_new(void);
DCP_API int dcp_press_open(struct dcp_press *, int gencode_id, char const *hmm,
                           char const *db);
DCP_API long dcp_press_nproteins(struct dcp_press const *);
DCP_API int dcp_press_next(struct dcp_press *);
DCP_API bool dcp_press_end(struct dcp_press const *);
DCP_API int dcp_press_close(struct dcp_press *);
DCP_API void dcp_press_del(struct dcp_press const *);

#endif
