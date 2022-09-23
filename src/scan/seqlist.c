#include "scan/seqlist.h"
#include "core/c23.h"
#include "core/errmsg.h"
#include "core/logging.h"
#include "imm/abc.h"
#include "jx.h"
#include "scan/seq.h"
#include "xfile.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

static JR_DECLARE(json, 128);
static char *data = nullptr;
static struct seq curr = {0};
static enum rc errnum = RC_OK;
static char errmsg[ERROR_SIZE] = {0};
static int64_t scan_id = 0;

enum rc seqlist_init(char const *filepath, struct imm_abc const *abc)
{
    JR_INIT(json);
    seq_init(&curr, abc);
    data = nullptr;

    int64_t size = 0;
    int r = xfile_readall(filepath, &size, (unsigned char **)&data);
    if (r)
    {
        errnum = eio(errfmt(errmsg, "reading seqs, %s", xfile_strerror(r)));
        goto cleanup;
    }

    assert(size <= INT_MAX);

    if (jr_parse(json, (int)size, data))
    {
        errnum = eparse(errfmt(errmsg, "failed to parse seqs json"));
        goto cleanup;
    }

    if (jr_type(json) != JR_ARRAY)
    {
        errnum = eparse(errfmt(errmsg, "wrong seqs file format"));
        goto cleanup;
    }

    if (jr_nchild(json) <= 0)
    {
        errnum = efail(errfmt(errmsg, "no sequence found"));
        goto cleanup;
    }

    jr_reset(json);
    jr_array_at(json, 0);
    scan_id = jr_long_of(json, "scan_id");
    if (jr_error())
    {
        errnum = eparse(errfmt(errmsg, "failed to get scan_id from seq file"));
        goto cleanup;
    }

    return RC_OK;

cleanup:
    seqlist_cleanup();
    return errnum;
}

int64_t seqlist_scan_id(void) { return scan_id; }

void seqlist_rewind(void)
{
    jr_reset(json);
    jr_array_at(json, 0);
}

struct seq const *seqlist_next(void)
{
    if (jr_type(json) == JR_SENTINEL) return nullptr;

    struct imm_abc const *abc = imm_seq_abc(&curr.iseq);
    curr.id = jr_long_of(json, "id");
    curr.name = jr_string_of(json, "name");
    curr.iseq = imm_seq(imm_str(jr_string_of(json, "data")), abc);

    jr_right(json);
    if (jr_error())
    {
        errnum = efail(errfmt(errmsg, "json: %s", jr_strerror(jr_error())));
        return nullptr;
    }

    return &curr;
}

void seqlist_cleanup(void)
{
    if (data)
    {
        free(data);
        data = nullptr;
    }
}
