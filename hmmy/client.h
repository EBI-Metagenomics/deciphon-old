#ifndef HMMY_CLIENT_H
#define HMMY_CLIENT_H

enum
{
    CLIENT_OFF,
    CLIENT_ON,
    CLIENT_FAIL,
};

void client_init(char const *exedir);
void client_start(void);
int client_state(void);
void client_stop(void);

#endif
