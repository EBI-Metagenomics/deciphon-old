#ifndef DECIPHON_PARTITION_H
#define DECIPHON_PARTITION_H

#include "deciphon/export.h"
#include <stdbool.h>
#include <stdint.h>

struct dcp_partition;
struct nmm_profile;

DCP_API struct dcp_partition*     dcp_partition_create(char const* filepath, uint64_t start_offset, uint32_t nmodels);
DCP_API struct nmm_profile const* dcp_partition_read(struct dcp_partition* part);
DCP_API int                       dcp_partition_reset(struct dcp_partition* part);
DCP_API bool                      dcp_partition_eof(struct dcp_partition const* part);
DCP_API uint32_t                  dcp_partition_nmodels(struct dcp_partition const* part);

#endif
