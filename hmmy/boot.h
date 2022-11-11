#ifndef HMMY_BOOT_H
#define HMMY_BOOT_H

#include <stdbool.h>

typedef void boot_end_fn_t(bool succeed);

void boot_init(void);
void boot_start(char const *hmm_file, boot_end_fn_t *);
void boot_stop(void);
bool boot_offline(void);
void boot_cleanup(void);

#endif
