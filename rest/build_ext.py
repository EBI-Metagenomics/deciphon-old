import os
from os.path import join

from cffi import FFI


ffibuilder = FFI()

folder = os.path.dirname(os.path.abspath(__file__))

with open(join(folder, "deciphon_rest", "interface.h"), "r") as f:
    ffibuilder.cdef(f.read())

ffibuilder.set_source(
    "deciphon_rest.csched",
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
