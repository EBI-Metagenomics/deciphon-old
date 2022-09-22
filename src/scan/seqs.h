#ifndef SCAN_SEQS_H
#define SCAN_SEQS_H

#include "core/limits.h"
#include "core/rc.h"
#include "imm/abc.h"
#include "imm/seq.h"
#include "jx.h"
#include <stdint.h>

struct seq
{
    int64_t id;
    char const *name;
    struct imm_seq iseq;
};

struct seqs
{
    JR_DECLARE(json, 128);
    char *data;
    int size;
    int64_t scan_id;
    struct imm_abc const *abc;
    struct seq curr;

    char errmsg[ERROR_SIZE];
};

enum rc seqs_init(struct seqs *seqs, char const *filepath,
                  struct imm_abc const *abc);
void seqs_rewind(struct seqs *seqs);
enum rc seqs_next(struct seqs *seqs, struct seq const **seq);
void seqs_cleanup(struct seqs *seqs);

#endif
