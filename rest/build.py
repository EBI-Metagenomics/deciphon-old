from cffi import FFI


ffibuilder = FFI()

ffibuilder.cdef(
    """
    struct sched_job
    {
        int64_t id;

        int64_t db_id;
        int32_t multi_hits;
        int32_t hmmer3_compat;
        char state[5];

        char error[256];
        int64_t submission;
        int64_t exec_started;
        int64_t exec_ended;
    };

    struct sched_db
    {
        int64_t id;
        char name[128];
    };

    typedef void logger_print_t(char const *msg, void *arg);
    extern "Python" void logger_print(char const *msg, void *arg);

    typedef void sched_db_peek_t(struct sched_db const *db, void *arg);
    extern "Python" void db_peek(struct sched_db const *db, void *arg);

    void logger_setup(logger_print_t *print, void *arg);

    enum rc sched_setup(char const *filepath);
    enum rc sched_open(void);
    enum rc sched_close(void);

    void    sched_job_init(struct sched_job *job, int64_t db_id,
                           bool multi_hits, bool hmmer3_compat);

    enum rc sched_db_list(sched_db_peek_t *peek, void *arg);
    enum rc sched_add_db(char const *filepath, int64_t *id);

    enum rc sched_submit_job(struct sched_job *job, char const *filepath, char *error);
"""
)

ffibuilder.set_source(
    "deciphon_rest._sched",
    """
     #include "common/logger.h"
     #include "common/limits.h"
     #include "sched/sched.h"
     #include "sched/db.h"
     #include "sched/job.h"
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
