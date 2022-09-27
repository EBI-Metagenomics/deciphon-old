#ifndef SCAN_SEQLIST_H
#define SCAN_SEQLIST_H

#include "core/rc.h"
#include <stdint.h>

struct imm_abc;

enum rc seqlist_init(char const *filepath, struct imm_abc const *);
int64_t seqlist_scan_id(void);
void seqlist_rewind(void);
struct seq const *seqlist_next(void);
int seqlist_size(void);
char const *seqlist_errmsg(void);
void seqlist_cleanup(void);

#endif