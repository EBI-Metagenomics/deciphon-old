#ifndef GENERICS_H
#define GENERICS_H

#include "protein_db.h"
#include "protein_model.h"
#include "protein_profile.h"
#include "standard_db.h"
#include "standard_profile.h"

#define dcp_super(x)                                                           \
    _Generic((x), struct dcp_standard_profile *                                        \
             : dcp_standard_profile_super, struct protein_prof *                       \
             : protein_profile_super, struct dcp_standard_db *                         \
             : dcp_standard_db_super, struct protein_db *                           \
             : protein_db_super)(x)

#define dcp_del(x)                                                             \
    _Generic((x), struct dcp_prof *                                            \
             : dcp_prof_del, struct dcp_standard_profile *                             \
             : dcp_standard_profile_del, struct protein_prof *                         \
             : protein_profile_del, struct protein_model *                        \
             : protein_model_del)(x)

#endif
