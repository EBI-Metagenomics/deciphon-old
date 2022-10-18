#ifndef DECY_BROKER_H
#define DECY_BROKER_H

#include "sched_structs.h"
#include <stdbool.h>

struct jr;
struct msg;

enum proc_id
{
    PRESSY_ID = 0,
    SCANNY_ID = 1,
    SCHEDY_ID = 2,
};

void broker_init(void);
char const *broker_forward_msg(char const *proc_name, struct msg *msg);
void broker_send(enum proc_id proc_id, char const *msg);
void broker_init(void);
void broker_terminate(void);

bool broker_parse_db(struct sched_db *, char *json);
bool broker_parse_hmm(struct sched_hmm *, char *json);
bool broker_parse_job(struct sched_job *, char *json);
bool broker_parse_scan(struct sched_scan *, char *json);
bool broker_parse_seq(struct sched_seq *, char *json);

#endif
