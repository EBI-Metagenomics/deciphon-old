#ifndef SEQLIST_H
#define SEQLIST_H

#include <stdbool.h>

struct seqlist *seqlist_new(void);
int seqlist_open(struct seqlist *, char const *filepath);
long seqlist_scan_id(struct seqlist const *);
void seqlist_rewind(struct seqlist *);
char const *seqlist_next(struct seqlist *);
bool seqlist_end(struct seqlist const *);
int seqlist_error(struct seqlist const *);
int seqlist_size(struct seqlist const *);
void seqlist_del(struct seqlist const *);

#endif
