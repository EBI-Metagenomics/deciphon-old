#ifndef SCAN_PRODFILE_LINE_H
#define SCAN_PRODFILE_LINE_H

#include "scan/prod.h"
#include "sizeof_field.h"

#define PRODFILE_LINE_SIZE                                                     \
    sizeof_field(struct prod, profile_name) +                                  \
        sizeof_field(struct prod, abc_name) +                                  \
        sizeof_field(struct prod, profile_typeid) +                            \
        sizeof_field(struct prod, version) +                                   \
        sizeof_field(struct prod, version)

#endif
