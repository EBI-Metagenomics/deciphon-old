#include "hmmer/result.h"
#include "h3c/h3c.h"
#include "logy.h"
#include <math.h>
#include <stdlib.h>

struct hmmer_result
{
    struct h3c_result *handle;
};

int hmmer_result_new(struct hmmer_result **x)
{
    if (!(*x = malloc(sizeof(struct hmmer_result))))
        return enomem("could not alloc hmmer_result");

    if (!((*x)->handle = h3c_result_new()))
    {
        free(*x);
        return enomem("could not alloc h3c_result");
    }

    return 0;
}
void hmmer_result_del(struct hmmer_result const *x)
{
    h3c_result_del(x->handle);
    free((void *)x);
}

unsigned hmmer_result_nhits(struct hmmer_result const *x)
{
    return h3c_result_nhits(x->handle);
}

double hmmer_result_evalue_ln(struct hmmer_result const *x)
{
    if (hmmer_result_nhits(x) == 0) return 0.0;
    return h3c_result_hit_evalue_ln(x->handle, 0);
}

double hmmer_result_evalue(struct hmmer_result const *x)
{
    return exp(hmmer_result_evalue_ln(x));
}

int hmmer_result_write(struct hmmer_result *x, FILE *fp)
{
    if (h3c_result_pack(x->handle, fp))
    {
        fclose(fp);
        return eio("could not write hmmer_result");
    }
    return 0;
}

int hmmer_result_read(struct hmmer_result *x, FILE *fp)
{
    if (h3c_result_unpack(x->handle, fp))
    {
        fclose(fp);
        return eio("could not read hmmer results");
    }
    return 0;
}

struct h3c_result *hmmer_result_handle(struct hmmer_result *x)
{
    return x->handle;
}
