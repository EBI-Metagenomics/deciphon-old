#include "uv/version.h"

#if UV_VERSION_MAJOR >= 1 && UV_VERSION_MINOR >= 44
#define HAS_AVAILABLE_PARALLELISM
#endif

#if defined(HAS_AVAILABLE_PARALLELISM)
#include "uv.h"
#else
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE 1
#endif
#include "uv.h"
#include <sched.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

int machine_ncpus(void)
{
#if defined(HAS_AVAILABLE_PARALLELISM)
    return (int)uv_available_parallelism();
#else
    cpu_set_t set;
    long rc;

    memset(&set, 0, sizeof(set));

    /* sysconf(_SC_NPROCESSORS_ONLN) in musl calls sched_getaffinity() but in
     * glibc it's... complicated... so for consistency try sched_getaffinity()
     * before falling back to sysconf(_SC_NPROCESSORS_ONLN).
     */
    if (0 == sched_getaffinity(0, sizeof(set), &set))
        rc = CPU_COUNT(&set);
    else
        rc = sysconf(_SC_NPROCESSORS_ONLN);

    if (rc < 1) rc = 1;

    return (int)rc;
#endif
}
