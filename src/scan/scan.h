#ifndef SCAN_SCAN_H
#define SCAN_SCAN_H

#include "core/rc.h"
#include "scan/cfg.h"
#include <stdbool.h>

void scan_init(struct scan_cfg cfg);
enum rc scan_setup(char const *db, char const *seqs);
enum rc scan_run(void);
char const *scan_errmsg(void);
char const *scan_prod_filepath(void);

#endif
