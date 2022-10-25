#include "scan/prodman.h"
#include "core/thrfiles.h"
#include "fs.h"

static struct thrfiles thrfiles = {0};

void prodman_init(void) { thrfiles_init(&thrfiles); }

bool prodman_setup(int nthreads) { return thrfiles_setup(&thrfiles, nthreads); }

static FILE *create_header(void);

char const *prodman_finishup(void)
{
    char const *path = thrfiles_finishup(&thrfiles);
    if (!path) return NULL;
    if (fs_sort(path)) return NULL;

    FILE *hdr = create_header();
    if (!hdr) return NULL;

    FILE *fp = fopen(path, "rb+");
    if (!fp) goto cleanup;
    if (fs_rjoin(hdr, fp)) goto cleanup;

    fclose(fp);
    fclose(hdr);
    return path;

cleanup:
    if (fp) fclose(fp);
    fclose(hdr);
    return NULL;
}

void prodman_cleanup(void);

static FILE *create_header(void)
{
    FILE *hdr = tmpfile();
    if (!hdr) return NULL;

    if (fputs("scan_id\tseq_id\tprofile_name\t", hdr) < 0) goto cleanup;
    if (fputs("abc_name\talt_loglik\t", hdr) < 0) goto cleanup;
    if (fputs("null_loglik\tprofile_typeid\t", hdr) < 0) goto cleanup;
    if (fputs("version\tmatch\n", hdr) < 0) goto cleanup;

    rewind(hdr);
    return hdr;

cleanup:
    fclose(hdr);
    return NULL;
}
