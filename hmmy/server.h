#ifndef HMMY_SERVER_H
#define HMMY_SERVER_H

enum
{
    SERVER_OFF,
    SERVER_ON,
    SERVER_FAIL,
};

void server_init(char const *podman_file);
void server_start(char const *hmm_file);
int server_state(void);
void server_stop(void);

#endif
