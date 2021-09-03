#ifndef DCP_PRO_DB_H
#define DCP_PRO_DB_H

#include "dcp/export.h"
#include "dcp/pro_cfg.h"
#include <stdio.h>

struct dcp_pro_db;

struct dcp_pro_db *dcp_pro_db_openr(FILE *restrict fd);

#endif
