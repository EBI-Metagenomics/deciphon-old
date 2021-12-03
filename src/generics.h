#ifndef GENERICS_H
#define GENERICS_H

#include "pro_db.h"
#include "pro_model.h"
#include "pro_prof.h"
#include "std_db.h"
#include "std_prof.h"

#define dcp_super(x)                                                           \
    _Generic((x), struct dcp_std_prof *                                        \
             : dcp_std_prof_super, struct dcp_pro_prof *                       \
             : dcp_pro_prof_super, struct dcp_std_db *                         \
             : dcp_std_db_super, struct dcp_pro_db *                           \
             : dcp_pro_db_super)(x)

#define dcp_del(x)                                                             \
    _Generic((x), struct dcp_prof *                                            \
             : dcp_prof_del, struct dcp_std_prof *                             \
             : dcp_std_prof_del, struct dcp_pro_prof *                         \
             : dcp_pro_prof_del, struct dcp_pro_model *                        \
             : dcp_pro_model_del)(x)

#endif
