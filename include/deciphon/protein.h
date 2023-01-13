#ifndef DECIPHON_PROTEIN_H
#define DECIPHON_PROTEIN_H

struct dcp_protein;

struct dcp_protein *dcp_protein_new(void);
void dcp_protein_del(struct dcp_protein const *);

#endif
