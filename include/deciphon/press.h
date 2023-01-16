#ifndef DECIPHON_PRESS_H
#define DECIPHON_PRESS_H

struct dcp_press;

struct dcp_press *dcp_press_new(void);
int dcp_press_open(struct dcp_press *, char const *hmm, char const *db);
long dcp_press_nsteps(struct dcp_press const *);
int dcp_press_step(struct dcp_press *);
int dcp_press_close(struct dcp_press *, int succesfully);
void dcp_press_del(struct dcp_press const *);

#endif
