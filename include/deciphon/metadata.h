#ifndef DECIPHON_METADATA_H
#define DECIPHON_METADATA_H

#include "deciphon/export.h"

struct dcp_metadata;

DCP_API struct dcp_metadata* dcp_metadata_create(char const* name, char const* acc);
DCP_API void                 dcp_metadata_destroy(struct dcp_metadata const* mt);
DCP_API char const*          dcp_metadata_get_acc(struct dcp_metadata const* mt);
DCP_API char const*          dcp_metadata_get_name(struct dcp_metadata const* mt);

#endif
