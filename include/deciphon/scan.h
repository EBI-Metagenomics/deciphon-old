#ifndef DECIPHON_SCAN_H
#define DECIPHON_SCAN_H

#include "deciphon/export.h"
#include <stdbool.h>

struct dcp_scan;

DCP_API struct dcp_scan *dcp_scan_new(void);
DCP_API void dcp_scan_set_nthreads(struct dcp_scan *, int nthreads);
DCP_API void dcp_scan_set_threshold(struct dcp_scan *, double lrt);
DCP_API void dcp_scan_set_multihits(struct dcp_scan *, bool);
DCP_API void dcp_scan_set_hmmer3compat(struct dcp_scan *, bool);
DCP_API int dcp_scan_set_database(struct dcp_scan *, char const *db);
DCP_API int dcp_scan_set_sequences(struct dcp_scan *, char const *seqs);
DCP_API int dcp_scan_run(struct dcp_scan *);
DCP_API void dcp_scan_del(struct dcp_scan const *);

#endif
