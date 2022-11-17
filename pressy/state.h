#ifndef STATE_H
#define STATE_H

enum state
{
    INIT,
    RUN,
    DONE,
    FAIL,
    CANCEL,
};

char const *state_string(enum state);

#endif
