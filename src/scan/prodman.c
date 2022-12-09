#include "scan/prodman.h"
#include "fs.h"
#include "logy.h"
#include "multifile.h"

static struct multifile multifile = {0};

void prodman_init(void) { multifile_init(&multifile); }

int prodman_setup(int nthreads)
{
    return multifile_setup(&multifile, nthreads);
}

FILE *prodman_file(int idx) { return multifile_file(&multifile, idx); }

static FILE *create_header(void);
static int join_header(FILE *hdr, char const *filepath);

int prodman_finishup(void)
{
    char const *path = NULL;
    int rc = multifile_finishup(&multifile, &path);
    if (rc) return rc;
    if (fs_sort(path)) return eio("failed to sort");

    FILE *hdr = create_header();
    if (!hdr) return eio("failed to create header file");
    rc = join_header(hdr, path);
    fclose(hdr);
    if (fs_mkdir("prod", true)) return eio("could not create dir");
    if (fs_move("prod/match.tsv", path)) return eio("could not move file");
    return rc;
}

void prodman_cleanup(void) { multifile_cleanup(&multifile); }

static FILE *create_header(void)
{
    FILE *hdr = tmpfile();
    if (!hdr) return NULL;

    if (fputs("scan_id\tseq_id\tprofile_name\t", hdr) < 0) goto cleanup;
    if (fputs("abc_name\talt_loglik\t", hdr) < 0) goto cleanup;
    if (fputs("null_loglik\tevalue_log\tprofile_typeid\t", hdr) < 0)
        goto cleanup;
    if (fputs("version\tmatch\n", hdr) < 0) goto cleanup;

    rewind(hdr);
    return hdr;

cleanup:
    fclose(hdr);
    return NULL;
}

static int join_header(FILE *hdr, char const *filepath)
{
    FILE *fp = fopen(filepath, "rb+");
    if (!fp) return eio("failed to fopen");
    if (fs_rjoin(hdr, fp))
    {
        fclose(fp);
        return eio("failed to join files");
    }
    fclose(fp);
    return RC_OK;
}
