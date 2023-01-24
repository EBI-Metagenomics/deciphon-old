#ifndef SEQLIST_H
#define SEQLIST_H

#include "json.h"
#include <stdbool.h>

struct seqlist
{
  long scan_id;
  char *data;
  bool end;
  int error;
  int size;
  struct json json[128];
};

void seqlist_init(struct seqlist *);
int seqlist_open(struct seqlist *, char const *filepath);
long seqlist_scan_id(struct seqlist const *);
void seqlist_rewind(struct seqlist *);
char const *seqlist_next(struct seqlist *);
bool seqlist_end(struct seqlist const *);
int seqlist_error(struct seqlist const *);
int seqlist_size(struct seqlist const *);
void seqlist_close(struct seqlist *);

#endif
