#ifndef DECIPHON_H3FILE_H
#define DECIPHON_H3FILE_H

#include <stdio.h>

struct dcp_h3file;

void dcp_h3file_init(struct dcp_h3file *);
int dcp_h3file_read(struct dcp_h3file *, FILE *);
int dcp_h3file_end(FILE *);

#endif
