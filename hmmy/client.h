#ifndef HMMY_CLIENT_H
#define HMMY_CLIENT_H

#include "state.h"
#include <stdbool.h>

void client_init(void);
void client_start(void);
void client_stop(void);
bool client_offline(void);
enum state client_state(void);
void client_cleanup(void);

#endif
