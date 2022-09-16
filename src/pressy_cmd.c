#include "pressy_cmd.h"
#include "core/c23.h"
#include "core/logging.h"
#include "core/pp.h"
#include "core/xfile.h"
#include "db/press.h"
#include "uv.h"
#include "zc.h"
#include <string.h>

static struct db_press db_press = {0};

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char hmm_filepath[PATH_MAX] = {0};
static char db_filepath[PATH_MAX] = {0};
static struct uv_work_s request = {0};
static struct uv_loop_s *loop = nullptr;

static cmd_fn_t *pressy_cmds[] = {
#define X(_, A) &A,
    PRESSY_CMD_MAP(X)
#undef X
};

static inline char const *say_ok(void) { return "OK"; }

static inline char const *say_fail(void) { return "FAIL"; }

static inline char const *say_yes(void) { return "YES"; }

static inline char const *say_no(void) { return "NO"; }

#define error_parse() error("failed to parse command")

static enum pressy_cmd parse(char const *);

cmd_fn_t *pressy_cmd(char const *cmd, struct uv_loop_s *loop_)
{
    loop = loop_;
    return pressy_cmds[parse(cmd)];
}

static void press_session(struct uv_work_s *);
static void press_cleanup(struct uv_work_s *, int status);

char const *pressy_cmd_invalid(struct cmd const *cmd) {}

char const *pressy_cmd_press(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "ss"))
    {
        error_parse();
        return say_fail();
    }

    char const *p = cmd->argv[1];
    if (zc_strlcpy(hmm_filepath, p, sizeof hmm_filepath) >= sizeof hmm_filepath)
    {
        enomem("file path is too long");
        return say_fail();
    }
    if (!xfile_is_readable(hmm_filepath))
    {
        eio("file is not readable");
        return say_fail();
    }

    press_session(0);
    // uv_queue_work(loop, &request, press_session, press_cleanup);
    return say_ok();
}

char const *pressy_cmd_state(struct cmd const *cmd) {}

static void press_session(struct uv_work_s *req)
{
    strcpy(db_filepath, hmm_filepath);
    db_filepath[strlen(db_filepath) - 3] = 'd';
    db_filepath[strlen(db_filepath) - 2] = 'c';
    db_filepath[strlen(db_filepath) - 1] = 'p';

    FILE *hmm = fopen(hmm_filepath, "rb");
    if (!hmm)
    {
        eio("failed to open hmm file");
        return;
    }

    FILE *db = fopen(db_filepath, "wb");
    if (!db)
    {
        eio("failed to open db file");
        fclose(hmm);
        return;
    }

    enum rc rc = db_press_init(&db_press, hmm, db);
    if (rc) return;
    unsigned nsteps = db_press_nsteps(&db_press);
    printf("Nsteps: %u\n", nsteps);
    fflush(stdout);

    while (!(rc = db_press_step(&db_press)))
        ;

    printf("Final RC1: %d\n", rc);
    rc = db_press_cleanup(&db_press);
    printf("Final RC2: %d\n", rc);

    fclose(hmm);
    fclose(db);
}

static void press_cleanup(struct uv_work_s *req, int status)
{
    printf("press_cleanup\n");
    fflush(stdout);
}

static enum pressy_cmd parse(char const *cmd)
{
#define X(A, _)                                                                \
    if (!strcmp(cmd, STRINGIFY(A))) return PRESSY_CMD_##A;
    PRESSY_CMD_MAP(X)
#undef X
    return PRESSY_CMD_INVALID;
}
