#ifndef DCP_GENERICS_H
#define DCP_GENERICS_H

#include "dcp/normal_profile.h"
#include "dcp/pro_profile.h"

#define dcp_super(x)                                                           \
    _Generic((x), struct dcp_normal_profile *                                  \
             : dcp_normal_profile_super, struct dcp_pro_profile *              \
             : dcp_pro_profile_super)(x)

#define dcp_del(x)                                                             \
    _Generic((x), struct dcp_profile *                                         \
             : dcp_profile_del, struct dcp_normal_profile *                    \
             : dcp_normal_profile_del, struct dcp_pro_profile *                \
             : dcp_pro_profile_del)(x)

#endif
