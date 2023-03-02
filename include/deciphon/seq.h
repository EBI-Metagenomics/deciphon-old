#ifndef DECIPHON_SEQ_H
#define DECIPHON_SEQ_H

#include "deciphon/export.h"
#include <stdbool.h>

struct dcp_seq;

typedef bool dcp_seq_next_fn(struct dcp_seq *, void *);

DCP_API void dcp_seq_setup(struct dcp_seq *, long id, char const *name,
                           char const *data);

#endif
