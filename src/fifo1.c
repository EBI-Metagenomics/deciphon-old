#include "fifo1.h"
#include <ck_fifo.h>
#include <stdlib.h>

struct fifo1
{
    struct ck_fifo_spsc*      fifo;
    struct ck_fifo_spsc_entry stub;
    int                       closed;
};

void fifo1_close(struct fifo1* fifo) { ck_pr_store_int(&fifo->closed, 1); }

bool fifo1_closed(struct fifo1* fifo) { return ck_pr_load_int(&fifo->closed); }

struct fifo1* fifo1_create(void)
{
    struct fifo1* fifo = malloc(sizeof(*fifo));
    fifo->fifo = malloc(sizeof(struct ck_fifo_spsc));
    ck_fifo_spsc_init(fifo->fifo, &fifo->stub);
    fifo->closed = false;
    return fifo;
}

void fifo1_destroy(struct fifo1* fifo)
{
    struct ck_fifo_spsc_entry* stub = NULL;
    ck_fifo_spsc_deinit(fifo->fifo, &stub);
    free(fifo);
}

bool fifo1_empty(struct fifo1* fifo) { return ck_fifo_spsc_isempty(fifo->fifo); }

struct fifo1_node* fifo1_pop(struct fifo1* fifo)
{
    struct fifo1_node* node = NULL;
    return ck_fifo_spsc_dequeue(fifo->fifo, &node) ? node : NULL;
}

void fifo1_push(struct fifo1* fifo, struct fifo1_node* new) { ck_fifo_spsc_enqueue(fifo->fifo, &new->entry, new); }
