#ifndef DB_WRITER_H
#define DB_WRITER_H

#include "cfg.h"
#include "entry_dist.h"
#include "lite_pack/lite_pack.h"
#include "protein.h"
#include "rc.h"
#include <stdio.h>

struct db_writer
{
  unsigned nprofiles;
  unsigned header_size;
  struct lip_file file;
  struct
  {
    struct lip_file header;
    struct lip_file prof_sizes;
    struct lip_file profiles;
  } tmp;

  struct imm_amino amino;
  struct imm_nuclt nuclt;
  struct imm_nuclt_code code;
  struct prot_cfg cfg;
};

int db_writer_open(struct db_writer *db, FILE *fp,
                   struct imm_amino const *amino, struct imm_nuclt const *nuclt,
                   struct prot_cfg cfg);

int db_writer_pack(struct db_writer *db, struct prot_prof const *profile);

int db_writer_close(struct db_writer *db, bool successfully);

#endif
