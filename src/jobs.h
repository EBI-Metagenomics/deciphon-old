#ifndef JOBS_H
#define JOBS_H

#include "abc.h"
#include "id.h"
#include <sqlite3.h>

struct dcp_jobs
{
    sqlite3 *db;
};

struct jobs_abc
{
    dcp_id id;
    char name[DCP_ABC_NAME_SIZE];
    dcp_utc creation;
    enum imm_abc_typeid type;
    struct imm_abc const *imm_abc;
    dcp_id user_id;
};

#define DCP_JOBS_INIT()                                                        \
    (struct dcp_jobs) { .db = NULL }

enum dcp_rc;
struct dcp_job;

enum dcp_rc jobs_add_job(struct dcp_jobs *jobs, unsigned user_id,
                         char const *sid, struct dcp_job *job);
enum dcp_rc jobs_add_db(struct dcp_jobs *jobs, unsigned user_id,
                        char const *name, char const *filepath,
                        char const *xxh3, char const *type);
enum dcp_rc jobs_setup(struct dcp_jobs *jobs, char const *filepath);
enum dcp_rc jobs_open(struct dcp_jobs *jobs, char const *filepath);
enum dcp_rc jobs_close(struct dcp_jobs *jobs);

#endif
