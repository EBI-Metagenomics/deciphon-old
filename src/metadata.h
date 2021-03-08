#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdio.h>

struct dcp_metadata const* profile_metadata_clone(struct dcp_metadata const* mt);
struct dcp_metadata const* profile_metadata_read(FILE* stream);
uint16_t                   profile_metadata_size(struct dcp_metadata const* mt);
int                        profile_metadata_write(struct dcp_metadata const* mt, FILE* stream);

#endif
