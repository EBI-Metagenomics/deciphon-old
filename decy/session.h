#ifndef DECY_SESSION_H
#define DECY_SESSION_H

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

void session_init(void);
char const *session_forward_msg(char const *proc_name, struct msg *msg);
void session_init(void);
void session_terminate(void);

bool session_parse_db(struct sched_db *, char *json);
bool session_parse_hmm(struct sched_hmm *, char *json);
bool session_parse_job(struct sched_job *, char *json);
bool session_parse_scan(struct sched_scan *, char *json);
bool session_parse_seq(struct sched_seq *, char *json);

#endif
