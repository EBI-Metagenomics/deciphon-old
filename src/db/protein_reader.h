#ifndef DECIPHON_PROTEIN_DB_READER_H
#define DECIPHON_PROTEIN_DB_READER_H

#include "db/reader.h"
#include "model/entry_dist.h"
#include "model/protein_cfg.h"
#include "model/protein_profile.h"

struct protein_db_reader
{
    struct db_reader super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct imm_nuclt_code code;
    struct protein_cfg cfg;
};

enum rc protein_db_reader_open(struct protein_db_reader *db, FILE *fp);

#endif
