#ifndef DB_PRESS_H
#define DB_PRESS_H

#include "db/prot_writer.h"
#include "model/prot_h3reader.h"
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

    unsigned profile_count;
    struct prot_profile profile;

    char buffer[4 * 1024];
};

enum rc db_press_init(struct db_press *, char const *hmm, char const *db);
long db_press_nsteps(struct db_press const *);
enum rc db_press_step(struct db_press *);
enum rc db_press_cleanup(struct db_press *, bool succesfully);

#endif
