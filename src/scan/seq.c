#include "scan/seq.h"
#include "core/c23.h"

void seq_init(struct seq *seq, struct imm_abc const *abc)
{
    seq->id = 0;
    seq->name = nullptr;
    seq->iseq = imm_seq(imm_str(""), abc);
}

void seq_set(struct seq *seq, long id, char const *name, char const *data)
{
    seq->id = id;
    seq->name = name;
    seq->iseq = imm_seq(imm_str(data), imm_seq_abc(&seq->iseq));
}

long seq_id(struct seq const *seq) { return seq->id; }

char const *seq_name(struct seq const *seq) { return seq->name; }

struct imm_seq const *seq_iseq(struct seq const *seq) { return &seq->iseq; }
