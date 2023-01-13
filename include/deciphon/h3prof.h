#ifndef DECIPHON_H3PROF_H
#define DECIPHON_H3PROF_H

#include <stdio.h>

struct dcp_h3prof;
struct dcp_prot_prof;

struct dcp_h3prof *dcp_h3prof_new(void);
int dcp_h3prof_convert(struct dcp_h3prof const *, struct dcp_prot_prof *);
void dcp_h3prof_del(struct dcp_h3prof const *);

#endif
