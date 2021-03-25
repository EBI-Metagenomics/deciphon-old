#ifndef SEQ_H
#define SEQ_H

#include "containers/snode.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct seq
{
    char const*  sequence;
    uint32_t     id;
    struct snode node;
};

static inline struct seq* seq_create(char const* sequence, uint32_t id);
static inline void        seq_destroy(struct seq* seq);
static inline uint32_t    seq_id(struct seq const* seq);
static inline char const* seq_string(struct seq const* seq);

static inline struct seq* seq_create(char const* sequence, uint32_t id)
{
    struct seq* seq = malloc(sizeof(*seq));
    seq->id = id;
    seq->sequence = strdup(sequence);
    snode_init(&seq->node);
    return seq;
}

static inline void seq_destroy(struct seq* seq)
{
    snode_deinit(&seq->node);
    free(seq);
}

static inline uint32_t seq_id(struct seq const* seq) { return seq->id; }

static inline char const* seq_string(struct seq const* seq) { return seq->sequence; }

#endif
