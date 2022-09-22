#ifndef SCAN_SEQS_H
#define SCAN_SEQS_H

#include "core/rc.h"
#include "jx.h"
#include <stdint.h>

struct seqs
{
    JR_DECLARE(json, 128);
    char *data;
    int size;
    int64_t scan_id;
};

enum rc seqs_init(struct seqs *seqs, char const *filepath);
void seqs_cleanup(struct seqs *seqs);

#endif
