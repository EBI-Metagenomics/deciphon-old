#include "core/limits.h"
#include "core/logging.h"
#include "core/progress.h"

struct thread
{
    unsigned long consumed;
    unsigned long past_consumed;
};

struct meter
{
    unsigned long range;
    unsigned long units;
};

static struct
{
    int num_threads;
    unsigned long total;
    unsigned long consumed;
    unsigned long past_consumed;
    struct thread thread[NUM_THREADS];

    struct meter meter;
    struct progress_callback callback;
} progress = {0};

void progress_setup(int num_threads, unsigned long total,
                    unsigned long meter_range,
                    struct progress_callback callback)
{
    progress.num_threads = num_threads;
    progress.total = total;
    progress.consumed = 0;
    progress.past_consumed = 0;
    progress.meter.range = meter_range;
    progress.meter.units = 0;
    for (int i = 0; i < num_threads; ++i)
    {
        progress.thread[i].consumed = 0;
        progress.thread[i].past_consumed = 0;
    }
    progress.callback = callback;
}

static void send_progress(unsigned long units)
{
    progress.callback.func(units, progress.callback.data);
}

static unsigned long update_meter_if_useful(void)
{
    unsigned long dlt = progress.consumed - progress.past_consumed;
    unsigned long units = (progress.meter.range * dlt) / progress.total;
    if (units >= 1)
    {
        progress.past_consumed += dlt;
        progress.meter.units += units;
    }
    return units;
}

static unsigned long estimate_units_increment(unsigned long dlt)
{
    return (progress.meter.range * dlt * progress.num_threads) / progress.total;
}

void progress_consume(int thread_id, unsigned long total)
{
    struct thread *t = progress.thread + thread_id;

    t->consumed += total;

    unsigned long dlt = (unsigned long)(t->consumed - t->past_consumed);
    if (estimate_units_increment(dlt) >= 1)
    {
        // info("Passed threshold for %d", thread_id);
        t->past_consumed += dlt;

        unsigned long units = 0;
        _Pragma ("omp critical")
        {
            progress.consumed += dlt;
            units = update_meter_if_useful();
        }
        if (units > 0)
        {
            // info("It is useful to send progress!");
            // _Pragma ("omp task")
            send_progress(units);
        }
    }
}

void progress_finishup(void)
{
    if (progress.consumed < progress.total) return;

    unsigned long units = progress.meter.range - progress.meter.units;
    if (units)
    {
        progress.meter.units += units;
        _Pragma ("omp task")
            send_progress(units);
    }
}
