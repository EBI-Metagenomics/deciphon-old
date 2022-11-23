#include "hmmer/client.h"
#include "deciphon_limits.h"
#include "h3c/h3c.h"
#include "hmmer/port.h"
#include "logy.h"
#include "loop/now.h"
#include "rc.h"

static int nstreams = 0;
static struct h3c_stream *streams[NUM_THREADS] = {0};

static inline long convert_deadline(long deadline)
{
    return h3c_deadline(deadline - now());
}

int hmmerc_start(int num_streams, long deadline)
{
    int rc = H3C_OK;

    struct h3c_dialer *dialer = h3c_dialer_new("127.0.0.1", HMMER_PORT);
    if (!dialer) enomem("could not allocate for hmmer client dialer");

    while (nstreams < num_streams)
    {
        if ((rc = h3c_dialer_dial(dialer, convert_deadline(deadline))))
        {
            rc = efail("hmmer client dial failure");
            goto cleanup;
        }
        streams[nstreams++] = h3c_dialer_stream(dialer);
    }

    h3c_dialer_del(dialer);
    return rc;

cleanup:
    hmmerc_stop();
    h3c_dialer_del(dialer);
    return rc;
}

int hmmerc_put(int id, char const *seq, long deadline)
{
    static char const cmd[] = "--hmmdb 1 --acc --cut_ga";
    int rc = h3c_stream_put(streams[id], cmd, seq, convert_deadline(deadline));
    if (rc) return efail("hmmer client put failure");
    return rc;
}

int hmmerc_pop(int id, double *ln_evalue)
{
    struct h3c_result *result = h3c_result_new();
    if (!result) return enomem("could not allocate for h3c results");

    h3c_stream_wait(streams[id]);
    int c = h3c_stream_pop(streams[id], result);
    if (c == H3C_ETIMEDOUT)
    {
        h3c_result_del(result);
        return efail("hmmer client pop: %s", h3c_decode(c));
    }

    if (h3c_result_nhits(result) == 0)
    {
        h3c_result_del(result);
        return efail("hmmer request has no hit");
    }

    *ln_evalue = h3c_result_hit_evalue_ln(result, 0);

    return H3C_OK;
}

void hmmerc_stop(void)
{
    for (int i = 0; i < nstreams; ++i)
    {
        h3c_stream_del(streams[i]);
        streams[i] = NULL;
    }
    nstreams = 0;
}
