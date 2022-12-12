#include "argless.h"
#include "deciphon_limits.h"
#include "filename.h"
#include "hmmer/client.h"
#include "logy.h"
#include "scan/scan.h"
#include <stdio.h>
#include <string.h>

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"disable-multi-hits", 'M', ARGL_FLAG(), "Disable multi-hits."},
    {"enable-hmmer3-compat", 'H', ARGL_FLAG(), "Enable hmmer3 compatibility."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = NULL,
                           .doc = "Scanny program.",
                           .version = "1.0.0"};

static void logger(char const *string, void *arg)
{
    FILE *fp = arg;
    fprintf(fp, "[%6s] ", "pressy");
    fputs(string, fp);
}

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl) != 2) argl_usage(&argl);
    int loglvl = argl_get(&argl, "loglevel")[0] - '0';

    zlog_setup(loglvl, logger, stderr);
    zlog_setroot("scanny");

    char const *db = argl_args(&argl)[1];
    if (filename_validate(db, "dcp")) goto fail;

    char const *seqs = argl_args(&argl)[0];

    static char hmm[FILENAME_SIZE] = {0};
    strcpy(hmm, db);
    filename_setext(hmm, "hmm");

    int multi_hits = !argl_has(&argl, "disable-multi-hits");
    int hmmer3_compat = argl_has(&argl, "enable-hmmer3-compat");

    printf("%s %s %d %d\n", seqs, db, multi_hits, hmmer3_compat);
    printf("%s\n", hmm);

    int nthreads = 1;
    struct scan_cfg cfg = {nthreads, 10., multi_hits, hmmer3_compat};
    scan_init(cfg);

    info("hmm_file: %s", hmm);
    info("db_file: %s", db);

    int rc = hmmer_client_start(nthreads, hmmer_client_deadline(5000));
    info("hmmer_client_start: %d", rc);

    rc = scan_setup(db, seqs);
    if (rc)
    {
        efail("%s", scan_errmsg());
        goto fail;
    }

    if ((rc = scan_run()))
    {
        efail("%s", scan_errmsg());
        goto fail;
    }

    if ((rc = scan_finishup()))
    {
        efail("%s", scan_errmsg());
        goto fail;
    }

    info("scan has finished");
    scan_cleanup();
    hmmer_client_stop();

    return EXIT_SUCCESS;

fail:
    scan_cleanup();
    hmmer_client_stop();
    printf("fail\n");
    return EXIT_FAILURE;
}
