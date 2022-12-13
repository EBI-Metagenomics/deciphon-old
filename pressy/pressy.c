#include "argless.h"
#include "db/press.h"
#include "filename.h"
#include "logy.h"
#include "progress.h"
#include <string.h>

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = NULL,
                           .doc = "Pressy program.",
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
    if (argl_nargs(&argl) != 1) argl_usage(&argl);
    int loglvl = argl_get(&argl, "loglevel")[0] - '0';

    zlog_setup(loglvl, logger, stderr);
    zlog_setroot("pressy");

    char const *hmm = argl_args(&argl)[0];
    if (filename_validate(hmm, "hmm")) goto fail;

    static char db[FILENAME_SIZE] = {0};
    strcpy(db, hmm);
    filename_setext(db, "dcp");

    static struct db_press db_press = {0};
    if (db_press_init(&db_press, hmm, db)) goto fail;

    static struct progress progress = {0};
    progress_init(&progress, db_press_nsteps(&db_press));
    info("found %ld profiles. Pressing...", db_press_nsteps(&db_press));

    int rc = 0;
    int last_progress = 0;
    while (!(rc = db_press_step(&db_press)))
    {
        progress_consume(&progress, 1);

        int current_progress = progress_percent(&progress);
        if (current_progress != last_progress)
        {
            printf("%d%%\n", current_progress);
            fflush(stdout);
            last_progress = current_progress;
        }
    }

    if (rc != RC_END)
    {
        db_press_cleanup(&db_press, false);
        goto fail;
    }

    if ((rc = db_press_cleanup(&db_press, true))) goto fail;

    info("pressing has finished");
    printf("done\n");

    return EXIT_SUCCESS;

fail:
    printf("fail\n");
    return EXIT_FAILURE;
}
