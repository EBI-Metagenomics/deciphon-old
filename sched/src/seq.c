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
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_i64(st, 0, seq->job_id)) return failed_to(EFAIL, "bind");
    if (xsql_bind_str(st, 1, seq->name)) return failed_to(EFAIL, "bind");
    if (xsql_bind_str(st, 2, seq->data)) return failed_to(EFAIL, "bind");

    if (xsql_step(st) != DONE) return failed_to(EFAIL, "step");
    seq->id = xsql_last_id(sched);
    return DONE;
}

static int next_seq_id(int64_t job_id, int64_t *seq_id)
{
    struct sqlite3_stmt *st = stmt[SEQ_SELECT_NEXT];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_i64(st, 0, *seq_id)) return failed_to(EFAIL, "bind");
    if (xsql_bind_i64(st, 1, job_id)) return failed_to(EFAIL, "bind");

    int rc = xsql_step(st);
    if (rc == DONE) return NOTFOUND;
    if (rc != NEXT) return failed_to(EFAIL, "get next seq id");
    *seq_id = sqlite3_column_int64(st, 0);

    if (xsql_step(st)) return failed_to(EFAIL, "step");
    return DONE;
}

static enum rc get_seq(struct sched_seq *seq)
{
    struct sqlite3_stmt *st = stmt[SEQ_SELECT];
    if (xsql_reset(st)) return failed_to(EFAIL, "reset");

    if (xsql_bind_i64(st, 0, seq->id)) return failed_to(EFAIL, "bind");

    if (xsql_step(st) != NEXT) failed_to(EFAIL, "get seq");

    seq->id = sqlite3_column_int64(st, 0);
    seq->job_id = sqlite3_column_int64(st, 1);

    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*seq, name))) return EFAIL;
    if (xsql_cpy_txt(st, 3, XSQL_TXT_OF(*seq, data))) return EFAIL;

    if (xsql_step(st)) return failed_to(EFAIL, "step");
    return DONE;
}

enum rc sched_seq_next(struct sched_seq *seq)
{
    int rc = next_seq_id(seq->job_id, &seq->id);
    if (rc == NOTFOUND) return DONE;
    if (rc != DONE) return EFAIL;
    if (get_seq(seq)) return EFAIL;
    return NEXT;
}
