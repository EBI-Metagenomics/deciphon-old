#ifndef SCAN_H
#define SCAN_H

struct dcp_result;
struct dcp_task_cfg;
struct nmm_profile;
struct seq;

void scan(struct nmm_profile const* profile, struct seq const* seq, struct dcp_result* result,
          struct dcp_task_cfg const* cfg);

#endif
