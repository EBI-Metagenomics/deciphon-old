#ifndef DECIPHON_SCAN_H
#define DECIPHON_SCAN_H

#include "deciphon/export.h"
#include "deciphon/seq.h"
#include <stdbool.h>

struct dcp_scan;

DCP_API struct dcp_scan *dcp_scan_new(int port);
DCP_API void dcp_scan_del(struct dcp_scan const *);

DCP_API int dcp_scan_set_nthreads(struct dcp_scan *, int nthreads);
DCP_API void dcp_scan_set_lrt_threshold(struct dcp_scan *, double);
DCP_API void dcp_scan_set_multi_hits(struct dcp_scan *, bool);
DCP_API void dcp_scan_set_hmmer3_compat(struct dcp_scan *, bool);
DCP_API void dcp_scan_set_heuristic(struct dcp_scan *, bool);

DCP_API int dcp_scan_set_db_file(struct dcp_scan *, char const *db);
DCP_API void dcp_scan_set_seq_iter(struct dcp_scan *, dcp_seq_next_fn *,
                                   void *);

DCP_API int dcp_scan_run(struct dcp_scan *, char const *name);

#endif
