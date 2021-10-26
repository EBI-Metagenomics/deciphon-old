#ifndef JOBS_H
#define JOBS_H

enum dcp_rc;
struct dcp_job;
struct dcp_jobs;

enum dcp_rc jobs_add_job(struct dcp_jobs *jobs, unsigned user_id,
                         char const *sid, struct dcp_job *job);
enum dcp_rc jobs_add_db(struct dcp_jobs *jobs, unsigned user_id,
                        char const *name, char const *filepath,
                        char const *xxh3, char const *type);
enum dcp_rc jobs_setup(struct dcp_jobs *jobs, char const *filepath);
enum dcp_rc jobs_open(struct dcp_jobs *jobs, char const *filepath);
enum dcp_rc jobs_close(struct dcp_jobs *jobs);

#endif
