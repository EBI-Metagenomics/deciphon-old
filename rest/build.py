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

    extern "Python" void logger_print(char const *msg, void *arg);

    typedef void logger_print_t(char const *msg, void *arg);
    void logger_setup(logger_print_t *print, void *arg);

    void    sched_job_init(struct sched_job *job, int64_t db_id,
                           bool multi_hits, bool hmmer3_compat);

    enum rc sched_add_db(char const *filepath, int64_t *id);

    enum rc sched_begin_job_submission(struct sched_job *job);
    void    sched_add_seq(struct sched_job *job, char const *name,
                          char const *data);
    enum rc sched_rollback_job_submission(struct sched_job *job);
    enum rc sched_end_job_submission(struct sched_job *job);
"""
)

ffibuilder.set_source(
    "_sched",
    """
     #include "common/logger.h"
     #include "common/limits.h"
     #include "sched/sched.h"
     #include "sched/job.h"
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
