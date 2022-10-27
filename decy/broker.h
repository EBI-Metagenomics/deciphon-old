#ifndef DECY_BROKER_H
#define DECY_BROKER_H

#include "sched_structs.h"
#include <stdbool.h>

struct msg;

enum pid
{
    PARENT_ID,
    PRESSY_ID,
    SCANNY_ID,
    SCHEDY_ID,
};

void broker_init(int64_t repeat);
void broker_send(enum pid pid, char const *msg);
void broker_terminate(void);

bool broker_parse_db(struct sched_db *, char *json);
bool broker_parse_hmm(struct sched_hmm *, char *json);
bool broker_parse_job(struct sched_job *, char *json);
bool broker_parse_scan(struct sched_scan *, char *json);
bool broker_parse_seq(struct sched_seq *, char *json);

#endif
