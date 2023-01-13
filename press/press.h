#ifndef PRESS_H
#define PRESS_H

#include "db_prot_writer.h"
#include "prot_h3reader.h"
#include <stdio.h>

struct db_press
{
  struct
  {
    FILE *fp;
    struct prot_db_writer db;
  } writer;

  struct
  {
    FILE *fp;
    struct prot_h3reader h3;
  } reader;

  unsigned prof_count;
  struct prot_prof prof;

  char buffer[4 * 1024];
};

int db_press_init(struct db_press *, char const *hmm, char const *db);
long db_press_nsteps(struct db_press const *);
int db_press_step(struct db_press *);
int db_press_cleanup(struct db_press *, bool succesfully);

#endif
