#ifndef DECIPHON_CORE_PROGRESS_H
#define DECIPHON_CORE_PROGRESS_H

struct progress_callback
{
    void (*func)(unsigned long units, void *data);
    void *data;
};

void progress_setup(int num_threads, unsigned long total,
                    unsigned long meter_range,
                    struct progress_callback callback);

void progress_consume(int thread_id, unsigned long total);

void progress_finishup(void);

#endif
