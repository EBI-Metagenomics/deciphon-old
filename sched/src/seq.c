#include "seq.h"
#include "common/rc.h"
#include "common/safe.h"
#include "stmt.h"
#include "xsql.h"
#include <sqlite3.h>

extern struct sqlite3 *sched;

void sched_seq_init(struct sched_seq *seq, int64_t job_id, char const *name,
                    char const *data)
{
    seq->id = 0;
    seq->job_id = job_id;
    safe_strcpy(seq->name, name, ARRAY_SIZE_OF(*seq, name));
    safe_strcpy(seq->data, data, ARRAY_SIZE_OF(*seq, data));
}

enum rc seq_submit(struct sched_seq *seq)
{
    struct sqlite3_stmt *st = stmt[SEQ_INSERT];
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, seq->job_id)) return efail("bind");
    if (xsql_bind_str(st, 1, seq->name)) return efail("bind");
    if (xsql_bind_str(st, 2, seq->data)) return efail("bind");

    if (xsql_step(st) != RC_DONE) return efail("step");
    seq->id = xsql_last_id(sched);
    return RC_DONE;
}

static int next_seq_id(int64_t job_id, int64_t *seq_id)
{
    struct sqlite3_stmt *st = stmt[SEQ_SELECT_NEXT];
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, *seq_id)) return efail("bind");
    if (xsql_bind_i64(st, 1, job_id)) return efail("bind");

    int rc = xsql_step(st);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) return efail("get next seq id");
    *seq_id = sqlite3_column_int64(st, 0);

    if (xsql_step(st)) return efail("step");
    return RC_DONE;
}

enum rc seq_get(struct sched_seq *seq)
{
#define ecpy efail("copy txt")

    struct sqlite3_stmt *st = stmt[SEQ_SELECT];
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, seq->id)) return efail("bind");

    enum rc rc = xsql_step(st);
    if (rc == RC_DONE) return RC_NOTFOUND;
    if (rc != RC_NEXT) efail("get seq");

    seq->id = sqlite3_column_int64(st, 0);
    seq->job_id = sqlite3_column_int64(st, 1);

    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*seq, name))) return ecpy;
    if (xsql_cpy_txt(st, 3, XSQL_TXT_OF(*seq, data))) return ecpy;

    if (xsql_step(st)) return efail("step");
    return RC_DONE;

#undef ecpy
}

enum rc sched_seq_next(struct sched_seq *seq)
{
    int rc = next_seq_id(seq->job_id, &seq->id);
    if (rc == RC_NOTFOUND) return RC_DONE;
    if (rc != RC_DONE) return efail("get next seq");
    if (seq_get(seq)) return efail("get next seq");
    return RC_NEXT;
}
