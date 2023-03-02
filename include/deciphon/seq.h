#ifndef DECIPHON_SEQ_H
#define DECIPHON_SEQ_H

#include "deciphon/export.h"

struct dcp_seq;

DCP_API void dcp_seq_setup(struct dcp_seq *, long id, char const *name,
                           char const *data);

#endif
