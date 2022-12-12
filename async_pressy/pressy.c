#include "argless.h"
#include "array_size.h"
#include "cmd.h"
#include "deciphon_limits.h"
#include "filename.h"
#include "fmt.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/parent.h"
#include "msg.h"
#include "pidfile.h"
#include "stringify.h"
#include "strlcpy.h"
#include "work.h"
#include <string.h>

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = NULL,
                           .doc = "Pressy program.",
                           .version = "1.0.0"};

static bool linger(void);
static void cleanup(void);

static void on_read(char *line);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));
    int loglvl = argl_get(&argl, "loglevel")[0] - '0';

    global_init(argv[0], loglvl);
    global_linger_setup(&linger, &cleanup);
    parent_init(&on_read, &global_shutdown, &global_shutdown);
    work_init();
    return global_run();
}

#define CMD_MAP(X)                                                             \
    X(quit, "")                                                                \
    X(echo, "[...]")                                                           \
    X(help, "")                                                                \
    X(cancel, "")                                                              \
    X(press, "HMM_FILE")                                                       \
    X(state, "")

#define X(A, _) static void A(struct msg *);
CMD_MAP(X)
#undef X

static struct cmd_entry entries[] = {
#define X(A, B) {&A, stringify(A), B},
    CMD_MAP(X)
#undef X
};

static void on_read(char *line)
{
    static struct msg msg = {0};
    if (msg_parse(&msg, line)) return;
    struct cmd_entry *e = cmd_find(array_size(entries), entries, msg_cmd(&msg));
    if (!e)
    {
        einval("unrecognized command: %s", msg_cmd(&msg));
        return;
    }

    msg_shift(&msg);
    (*e->func)(&msg);
}

static bool linger(void)
{
    if (work_state() != RUN) parent_close();
    return parent_closed() && work_state() != RUN;
}

static void cleanup(void)
{
    work_cancel();
    parent_close();
}

static void quit(struct msg *msg)
{
    unused(msg);
    global_shutdown();
}

static void echo(struct msg *msg) { parent_send(msg_unparse(msg)); }

static void help(struct msg *msg)
{
    unused(msg);
    cmd_help_init();

    for (size_t i = 0; i < array_size(entries); ++i)
        cmd_help_add(entries[i].name, entries[i].doc);

    parent_send(cmd_help_table());
}

static void cancel(struct msg *msg)
{
    work_cancel();
    parent_send(msg_ctx(msg, "ok"));
}

static void press(struct msg *msg)
{
    if (msg_check(msg, "s"))
    {
        parent_send(msg_ctx(msg, "fail"));
        return;
    }
    if (work_state() == RUN)
    {
        parent_send(msg_ctx(msg, "busy"));
        return;
    }

    char const *hmm = msg_str(msg, 0);
    if (filename_validate(hmm, "hmm"))
    {
        parent_send(msg_ctx(msg, "fail"));
        return;
    }

    char db[FILENAME_SIZE] = {0};
    if (strlcpy(db, hmm, sizeof(db)) >= sizeof(db))
    {
        enomem("file name is too long");
        parent_send(msg_ctx(msg, "fail"));
        return;
    }

    if (filename_setext(db, "dcp"))
    {
        parent_send(msg_ctx(msg, "fail"));
        return;
    }

    if (work_run(hmm, db))
    {
        parent_send(msg_ctx(msg, "fail"));
        return;
    }
    parent_send(msg_ctx(msg, "ok"));
}

static void state(struct msg *msg)
{
    char perc[] = "100%";
    fmt_perc(perc, work_progress());
    strcat(perc, "%");
    parent_send(msg_ctx(msg, "ok", state_string(work_state()), perc,
                        work_hmmfile(), work_dbfile()));
}
