#include "hmmer/client.h"
#include "deciphon_limits.h"
#include "h3c/h3c.h"
#include "hmmer/port.h"
#include "hmmer/result.h"
#include "logy.h"
#include "loop/now.h"
#include "rc.h"
#include <string.h>

static int nstreams = 0;
static struct h3c_stream *streams[NUM_THREADS] = {0};

static inline long convert_deadline(long deadline)
{
    return h3c_deadline(deadline - now());
}

int hmmer_client_start(int num_streams, long deadline)
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
    hmmer_client_stop();
    h3c_dialer_del(dialer);
    return rc;
}

#define CMD_SIZE 128
static void cmd_set(char cmd[CMD_SIZE], int hmm_idx);

int hmmer_client_put(int id, int hmm_idx, char const *seq, long deadline)
{
    char cmd[128] = {0};
    cmd_set(cmd, hmm_idx);
    int rc = h3c_stream_put(streams[id], cmd, "none", seq,
                            convert_deadline(deadline));
    if (rc) return efail("hmmer client put failure");
    return rc;
}

int hmmer_client_pop(int id, struct hmmer_result *r)
{
    h3c_stream_wait(streams[id]);
    int c = h3c_stream_pop(streams[id], hmmer_result_handle(r));
    if (c == H3C_ETIMEDOUT) return etimeout("hmmer client pop");
    return c ? efail("hmmer client pop: %s", h3c_decode(c)) : 0;
}

void hmmer_client_stop(void)
{
    for (int i = 0; i < nstreams; ++i)
    {
        h3c_stream_del(streams[i]);
        streams[i] = NULL;
    }
    nstreams = 0;
}

static void cmd_set(char cmd[CMD_SIZE], int hmm_idx)
{
    sprintf(cmd, "--hmmdb 1 --hmmdb_range %d..%d", hmm_idx, hmm_idx);
    strcat(cmd, " --acc --cut_ga");
}
