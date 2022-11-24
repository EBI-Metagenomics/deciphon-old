#ifndef PROT_DB_READER_H
#define PROT_DB_READER_H

#include "db/reader.h"
#include "model/entry_dist.h"
#include "model/prot_cfg.h"
#include "model/prot_profile.h"

struct prot_db_reader
{
    struct db_reader super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct imm_nuclt_code code;
    struct prot_cfg cfg;
};

enum rc prot_db_reader_open(struct prot_db_reader *db, FILE *fp);

#endif
