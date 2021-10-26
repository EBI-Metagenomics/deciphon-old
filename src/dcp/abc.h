#ifndef DCP_ABC_H
#define DCP_ABC_H

#include "dcp/export.h"
#include "dcp/id.h"
#include "dcp/strlcpy.h"
#include "dcp/utc.h"
#include "imm/imm.h"
#include <stdint.h>

#define DCP_ABC_NAME_SIZE 16

struct dcp_abc
{
    dcp_id id;
    char name[DCP_ABC_NAME_SIZE];
    dcp_utc creation;
    enum imm_abc_typeid type;
    struct imm_abc const *imm_abc;
    dcp_id user_id;
};

static inline void dcp_abc_init(struct dcp_abc *abc,
                                enum imm_abc_typeid type,
                                struct imm_abc const *imm_abc)
{
    abc->id = DCP_ID_NULL;
    abc->name[0] = '\0';
    abc->creation = DCP_UTC_NULL;
    abc->type = type;
    abc->imm_abc = imm_abc;
    abc->user_id = DCP_ID_NULL;
}

#endif
