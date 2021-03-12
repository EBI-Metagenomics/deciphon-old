#ifndef RESULTS_H
#define RESULTS_H

struct dcp_result;
struct dcp_results;

void                results_add(struct dcp_results* results, struct dcp_result* result);
struct dcp_results* results_create(void);

#endif
