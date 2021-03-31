#ifndef CLOCK_H
#define CLOCK_H

struct clock;

struct clock* clock_create(void);
void          clock_destroy(struct clock* clock);
void          clock_sleep(struct clock* clock, unsigned milliseconds);
void          clock_wakeup(struct clock* clock);

#endif
