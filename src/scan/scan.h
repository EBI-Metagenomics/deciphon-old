#ifndef SCAN_SCAN_H
#define SCAN_SCAN_H

#include "core/rc.h"
#include "scan/cfg.h"
#include <stdbool.h>

struct progress;

void scan_init(struct scan_cfg cfg);
int scan_setup(char const *db, char const *seqs);
int scan_run(void);
int scan_progress_update(void);
struct progress const *scan_progress(void);
char const *scan_errmsg(void);
int scan_finishup(char const **filepath);
void scan_cleanup(void);

#endif
