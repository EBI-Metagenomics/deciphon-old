#include "xfile.h"
#include "compiler.h"
#include "dcp_sched/rc.h"
#include "safe.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <xxhash.h>

#define BUFFSIZE (8 * 1024)
