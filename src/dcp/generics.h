#ifndef DCP_GENERICS_H
#define DCP_GENERICS_H

#include "dcp/pro_profile.h"
#include "dcp/std_profile.h"

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
