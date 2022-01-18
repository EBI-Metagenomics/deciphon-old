from cffi import FFI
import os
from os.path import join


ffibuilder = FFI()

# typedef void sched_db_peek_t(struct sched_db const *db, void *arg);
# extern "Python" void db_peek(struct sched_db const *db, void *arg);
# enum rc sched_db_list(sched_db_peek_t *peek, void *arg);

folder = os.path.dirname(os.path.abspath(__file__))

with open(join(folder, "deciphon_rest", "interface.h"), "r") as f:
    ffibuilder.cdef(f.read())

ffibuilder.set_source(
    "deciphon_rest._csched",
    """
     #include "common/logger.h"
     #include "common/limits.h"
     #include "sched/sched.h"
     #include "sched/db.h"
     #include "sched/job.h"
     #include "sched/seq.h"
     #include "sched/prod.h"
""",
    language="c",
    libraries=["sched_bundled"],
    include_dirs=[
        "/Users/horta/code/deciphon/build/common",
        "/Users/horta/code/deciphon/common/include",
        "/Users/horta/code/deciphon/sched/include",
    ],
    library_dirs=[
        "/Users/horta/code/deciphon/build",
    ],
)


if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
