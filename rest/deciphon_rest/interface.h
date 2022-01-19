FILE *fdopen(int, const char *);
int fclose(FILE *);

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
    int64_t xxh64;
    char filepath[4096];
};

struct sched_seq
{
    int64_t id;
    int64_t job_id;
    char name[256];
    char data[1048576];
};

struct sched_prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;

    char profile_name[64];
    char abc_name[16];

    double alt_loglik;
    double null_loglik;

    char profile_typeid[16];
    char version[16];

    char match[5242880];
};

typedef void logger_print_t(char const *msg, void *arg);
extern "Python" void logger_callback(char const *msg, void *arg);

void logger_setup(logger_print_t *print, void *arg);

enum rc sched_setup(char const *filepath);
enum rc sched_open(void);
enum rc sched_close(void);

void sched_job_init(struct sched_job *job, int64_t db_id, bool multi_hits,
                    bool hmmer3_compat);

enum rc sched_add_db(char const *filepath, int64_t *id);

enum rc sched_submit_job(struct sched_job *job, char const *filepath,
                         char *error);

enum sched_job_state
{
    SCHED_JOB_PEND,
    SCHED_JOB_RUN,
    SCHED_JOB_DONE,
    SCHED_JOB_FAIL
};

enum rc sched_job_state(int64_t job_id, enum sched_job_state *state);

enum rc sched_get_seq(struct sched_seq *seq);

enum rc sched_next_pend_job(struct sched_job *job);
enum rc sched_next_pending_job(struct sched_job *job);

enum rc sched_seq_next(struct sched_seq *seq);

enum rc sched_cpy_db_filepath(unsigned size, char *filepath, int64_t id);

enum rc sched_get_job(struct sched_job *job);

enum rc sched_get_db(struct sched_db *db);

enum rc sched_get_prod(struct sched_prod *prod);

typedef void(sched_prod_set_cb)(struct sched_prod *prod, void *arg);
extern "Python" void prod_set_cb(struct sched_prod *prod, void *arg);

enum rc sched_get_job_prods(int64_t job_id, sched_prod_set_cb cb,
                            struct sched_prod *prod, void *arg);

typedef void(sched_seq_set_cb)(struct sched_seq *seq, void *arg);
extern "Python" void seq_set_cb(struct sched_seq *seq, void *arg);

enum rc sched_get_job_seqs(int64_t job_id, sched_seq_set_cb cb,
                           struct sched_seq *seq, void *arg);

typedef bool(sched_seq_get_cb)(struct sched_seq **seq, void *arg);
extern "Python" bool seq_get_cb(struct sched_seq **seq, void *arg);

enum rc sched_submit_job2(struct sched_job *job, sched_seq_get_cb cb,
                          void *arg);

enum rc sched_begin_job_submission(struct sched_job *job);
void sched_add_seq(struct sched_job *job, char const *name, char const *data);
enum rc sched_rollback_job_submission(struct sched_job *job);
enum rc sched_end_job_submission(struct sched_job *job);

enum rc sched_prod_submit(struct sched_prod *prod);

enum rc sched_set_job_run(int64_t job_id);

enum rc sched_submit_prod_file(FILE *fp);
