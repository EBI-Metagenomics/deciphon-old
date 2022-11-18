#include "scan/seqlist.h"
#include "core/errmsg.h"
#include "core/strings.h"
#include "fs.h"
#include "imm/abc.h"
#include "jx.h"
#include "logy.h"
#include "scan/seq.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

static JR_DECLARE(parser, 128);
static char *data = NULL;
static int num_seqs = 0;
static struct seq curr = {0};
static enum rc errnum = RC_OK;
static char errmsg[ERROR_SIZE] = {0};
static long scan_id = 0;

enum rc seqlist_init(char const *filepath, struct imm_abc const *abc)
{
    JR_INIT(parser);
    seq_init(&curr, abc);
    data = NULL;
    num_seqs = 0;
    errnum = RC_OK;
    errmsg[0] = '\0';

    long size = 0;
    int r = fs_readall(filepath, &size, (unsigned char **)&data);
    if (r)
    {
        errnum = eio("%s", errfmt(errmsg, "%s", fs_strerror(r)));
        goto cleanup;
    }

    assert(size <= INT_MAX);

    if (jr_parse(parser, (int)size, data))
    {
        errnum = eparse("%s", errfmt(errmsg, FAIL_PARSE_JSON));
        goto cleanup;
    }

    if (jr_type(parser) != JR_ARRAY)
    {
        errnum = eparse("%s", errfmt(errmsg, "wrong seqs file format"));
        goto cleanup;
    }

    if ((num_seqs = jr_nchild(parser)) <= 0)
    {
        errnum = efail("%s", errfmt(errmsg, "no sequence found"));
        goto cleanup;
    }

    jr_reset(parser);
    jr_array_at(parser, 0);
    scan_id = jr_long_of(parser, "scan_id");
    if (jr_error())
    {
        errnum = eparse("%s", errfmt(errmsg, "failed to get scan_id"));
        goto cleanup;
    }

    return RC_OK;

cleanup:
    seqlist_cleanup();
    return errnum;
}

long seqlist_scan_id(void) { return scan_id; }

void seqlist_rewind(void)
{
    jr_reset(parser);
    jr_array_at(parser, 0);
}

struct seq const *seqlist_next(void)
{
    if (jr_type(parser) == JR_SENTINEL) return NULL;

    struct imm_abc const *abc = imm_seq_abc(&curr.iseq);
    curr.id = jr_long_of(parser, "id");
    curr.name = jr_string_of(parser, "name");
    curr.iseq = imm_seq(imm_str(jr_string_of(parser, "data")), abc);

    jr_right(parser);
    if (jr_error())
    {
        int rc = jr_error();
        errnum = efail("%s", errfmt(errmsg, "parser: %s", jr_strerror(rc)));
        return NULL;
    }

    return &curr;
}

int seqlist_size(void) { return num_seqs; }

char const *seqlist_errmsg(void) { return errmsg; }

void seqlist_cleanup(void)
{
    if (data)
    {
        free(data);
        data = NULL;
    }
}
