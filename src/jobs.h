#ifndef JOBS_H
#define JOBS_H

enum dcp_rc;
struct dcp_jobs;

enum dcp_rc jobs_add_db(struct dcp_jobs *jobs, char const *filepath);
enum dcp_rc jobs_setup(struct dcp_jobs *jobs, char const *filepath);
enum dcp_rc jobs_close(struct dcp_jobs *jobs);

#endif
