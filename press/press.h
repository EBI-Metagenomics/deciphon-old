#ifndef PRESS_H
#define PRESS_H

#include "db_writer.h"
#include "h3reader.h"
#include <stdio.h>

struct db_press
{
  struct
  {
    FILE *fp;
    struct db_writer db;
  } writer;

  struct
  {
    FILE *fp;
    struct h3reader h3;
  } reader;

  unsigned prof_count;
  struct protein prof;

  char buffer[4 * 1024];
};

int db_press_init(struct db_press *, char const *hmm, char const *db);
long db_press_nsteps(struct db_press const *);
int db_press_step(struct db_press *);
int db_press_cleanup(struct db_press *, bool succesfully);

#endif