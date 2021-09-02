#ifndef SCAN_H
#define SCAN_H

struct dcp_prof;
struct dcp_result;
struct dcp_task_cfg;
struct seq;

int scan(struct dcp_prof const *profile, struct seq const *seq,
         struct dcp_result *result, struct dcp_task_cfg const *cfg);

#endif
