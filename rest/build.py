from cffi import FFI


ffibuilder = FFI()

ffibuilder.cdef(
    """
    extern "Python" void logger_print(char const *msg, void *arg);

    typedef void logger_print_t(char const *msg, void *arg);
    void logger_setup(logger_print_t *print, void *arg);

    enum rc sched_add_db(char const *filepath, int64_t *id);
"""
)

ffibuilder.set_source(
    "_sched",
    """
     #include "common/logger.h"
     #include "sched/sched.h"
""",
    language="c",
    libraries=["sched"],
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
