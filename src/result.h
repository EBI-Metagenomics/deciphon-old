#ifndef RESULT_H
#define RESULT_H

#include "dcp/dcp.h"
#include "model.h"
#include "util.h"

struct dcp_results;

struct dcp_result
{
    struct dcp_results const* parent;
    uint32_t                  profid;
    uint32_t                  seqid;
    struct model              models[ARRAY_SIZE(dcp_models)];
};

void                        result_deinit(struct dcp_result const* result);
void                        result_init(struct dcp_result* result, struct dcp_results const* parent);
static inline struct model* result_model(struct dcp_result* r, enum dcp_model model) { return r->models + model; }
static inline void          result_set_profid(struct dcp_result* r, uint32_t profid) { r->profid = profid; }
static inline void          result_set_seqid(struct dcp_result* r, uint32_t seqid) { r->seqid = seqid; }

#endif
