#ifndef DECIPHON_SEQ_ITER_H
#define DECIPHON_SEQ_ITER_H

#include "deciphon/export.h"
#include <stdbool.h>

struct dcp_seq_iter;
struct dcp_seq;

typedef bool dcp_seq_iter_next_callb(struct dcp_seq *, void *);

DCP_API struct dcp_seq_iter *dcp_seq_iter_new(dcp_seq_iter_next_callb *,
                                              void *);
DCP_API bool dcp_seq_iter_next(struct dcp_seq_iter *x);
DCP_API void dcp_seq_iter_del(struct dcp_seq_iter const *);

#endif
