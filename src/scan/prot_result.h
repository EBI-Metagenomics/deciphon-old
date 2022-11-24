#ifndef SCAN_PROT_RESULT_H
#define SCAN_PROT_RESULT_H

#include "imm/imm.h"
#include "scan/prot_match.h"
#include "sched_structs.h"

struct prot_result
{
    struct prot_profile const *profile;
    struct imm_seq const *seq;
    struct imm_path const *path;
    struct prot_match match;
    char amino_seq[SCHED_SEQ_SIZE];
};

struct prot_profile;

void prot_result_init(struct prot_result *, struct prot_profile const *,
                      struct imm_seq const *, struct imm_path const *);
void prot_result_query_hmmer3(struct prot_result *);

#endif
