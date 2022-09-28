#ifndef DECY_CFG_H
#define DECY_CFG_H

void cfg_init(void);
int cfg_nthreads(void);
char const *cfg_host(void);
int cfg_port(void);
char const *cfg_prefix(void);
char const *cfg_uri(void);
char const *cfg_key(void);

#endif
