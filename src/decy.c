#include "deciphon/core/liner.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/looper.h"
#include <assert.h>
#include <stdbool.h>

static struct looper looper = {0};

void benchy_init();
void schedy_init();
void hmmy_init();

void decy_init(void)
{
    looper_init(&looper);
    benchy_init();
    schedy_init();
    hmmy_init();
}

void decy_run(void)
{
    looper_run(&looper);
    looper_cleanup(&looper);
}
