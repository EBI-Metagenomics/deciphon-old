#ifndef SEQ_LIST_H
#define SEQ_LIST_H

#include "json.h"
#include <stdbool.h>
#include <stdio.h>

struct seq_list
{
  char filename[FILENAME_MAX];
  long scan_id;
  char *data;
  bool end;
  int error;
  int size;
  struct json json[128];
};

void seq_list_init(struct seq_list *);
int seq_list_open(struct seq_list *);
void seq_list_close(struct seq_list *);

int seq_list_set_filename(struct seq_list *, char const *);
long seq_list_scan_id(struct seq_list const *);
void seq_list_rewind(struct seq_list *);
char const *seq_list_next(struct seq_list *);
bool seq_list_end(struct seq_list const *);
int seq_list_error(struct seq_list const *);
int seq_list_size(struct seq_list const *);

#endif