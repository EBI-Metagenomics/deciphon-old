#ifndef DECIPHON_PROTEIN_DB_WRITER_H
#define DECIPHON_PROTEIN_DB_WRITER_H

#include "deciphon/core/rc.h"
#include "deciphon/model/entry_dist.h"
#include "deciphon/model/protein_cfg.h"
#include "deciphon/model/protein_profile.h"
#include "writer.h"
#include <stdio.h>

struct protein_db_writer
{
    struct db_writer super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct imm_nuclt_code code;
    struct protein_cfg cfg;
};

enum rc protein_db_writer_open(struct protein_db_writer *db, FILE *fp,
                               struct imm_amino const *amino,
                               struct imm_nuclt const *nuclt,
                               struct protein_cfg cfg);

enum rc protein_db_writer_pack_profile(struct protein_db_writer *db,
                                       struct protein_profile const *profile);

#endif
