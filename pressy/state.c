#include "state.h"
#include "core/pp.h"
#include <assert.h>

static char const *strings[] = {[INIT] = "init",
                                [RUN] = "run",
                                [DONE] = "done",
                                [FAIL] = "fail",
                                [CANCEL] = "cancel"};

char const *state_string(enum state state)
{
    assert(state >= 0 && state < array_size(strings));
    return strings[state];
}
