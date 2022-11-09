#ifndef HMMY_CLIENT_H
#define HMMY_CLIENT_H

#include "state.h"

void client_init(void);
void client_start(void);
void client_stop(void);
enum state client_state(void);
void client_cleanup(void);

#endif
