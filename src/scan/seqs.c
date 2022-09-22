#include "scan/seqs.h"
#include "core/c23.h"
#include "core/logging.h"
#include "xfile.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum rc seqs_init(struct seqs *seqs, char const *filepath)
{
    seqs->data = nullptr;
    JR_INIT(seqs->json);

    int64_t size = 0;
    int r = xfile_readall(filepath, &size, (unsigned char **)&seqs->data);
    if (r) return eio(xfile_strerror(r));

    assert(size <= INT_MAX);

    if (jr_parse(seqs->json, strlen(seqs->data), seqs->data))
    {
        seqs_cleanup(seqs);
        return eparse("failed to parse seqs json");
    }

    if (jr_type(seqs->json) != JR_ARRAY)
    {
        seqs_cleanup(seqs);
        return eparse("wrong seqs file format");
    }

    int nseqs = jr_nchild(seqs->json);
    if (nseqs <= 0)
    {
        seqs_cleanup(seqs);
        return efail("no sequence found");
    }

    jr_reset(seqs->json);
    jr_array_at(seqs->json, 0);
    seqs->scan_id = jr_long_of(seqs->json, "scan_id");
    if (jr_error())
    {
        seqs_cleanup(seqs);
        return eparse("failed to fetch scan_id from seq file");
    }

    seqs->size = nseqs;

    return RC_OK;
}

void seqs_cleanup(struct seqs *seqs)
{
    if (seqs->data)
    {
        free(seqs->data);
        seqs->data = nullptr;
    }
}
