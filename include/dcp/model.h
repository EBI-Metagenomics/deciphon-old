#ifndef DCP_MODEL_H
#define DCP_MODEL_H

#include "dcp/export.h"

enum dcp_model
{
    DCP_MODEL_ALT = 0,
    DCP_MODEL_NULL = 1
};

static enum dcp_model const dcp_models[2] = {DCP_MODEL_ALT, DCP_MODEL_NULL};

#endif
