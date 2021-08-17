#ifndef DCP_GENERICS_H
#define DCP_GENERICS_H

#include "dcp/pro/profile.h"
#include "dcp/std_profile.h"

#define dcp_super(x)                                                           \
    _Generic((x), struct dcp_std_profile *                                     \
             : dcp_std_profile_super, struct dcp_pro_profile *                 \
             : dcp_pro_profile_super)(x)

#define dcp_del(x)                                                             \
    _Generic((x), struct dcp_profile *                                         \
             : dcp_profile_del, struct dcp_std_profile *                       \
             : dcp_std_profile_del, struct dcp_pro_profile *                   \
             : dcp_pro_profile_del, struct dcp_pro_model *                     \
             : dcp_pro_model_del)(x)

#endif
