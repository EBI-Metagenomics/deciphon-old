#include "boot.h"
#include "client.h"
#include "logy.h"
#include "loop/global.h"
#include "server.h"
#include <uv.h>

enum boot_state
{
    BOOT_INIT,
    BOOT_SERVER,
    BOOT_CLIENT,
    BOOT_DONE,
    BOOT_FAIL,
    BOOT_CANCEL,
};

static boot_end_fn_t *boot_end = NULL;
static uv_timer_t timer = {0};
static enum boot_state state = BOOT_INIT;

void boot_init(void)
{
    if (uv_timer_init(global_loop(), &timer)) fatal("failed to init timer");
}

static void boot_cycle(struct uv_timer_s *req);

void boot_start(char const *hmm_file, boot_end_fn_t *boot_end_fn)
{
    if (state != BOOT_INIT && state != BOOT_FAIL) boot_stop();
    boot_end = boot_end_fn;
    if (uv_timer_start(&timer, &boot_cycle, 1000, 100))
        fatal("failed to start timer");
    state = BOOT_SERVER;
    server_start(hmm_file);
}

void boot_stop(void)
{
    if (uv_timer_stop(&timer)) error("failed to stop timer");
    if (state == BOOT_SERVER) server_cancel();
    if (state == BOOT_CLIENT) client_stop();
}

bool boot_offline(void) { return state == BOOT_INIT || state == BOOT_FAIL; }

void boot_cleanup(void)
{
    if (uv_timer_stop(&timer)) error("failed to stop timer");
    uv_close((struct uv_handle_s *)&timer, NULL);
    state = BOOT_INIT;
}

static void boot_cycle(struct uv_timer_s *req)
{
    (void)req;

    if (state == BOOT_SERVER && server_state() == ON)
    {
        state = BOOT_CLIENT;
        client_start();
    }
    else if (state == BOOT_SERVER && server_state() == FAIL)
    {
        state = BOOT_FAIL;
        boot_stop();
        (*boot_end)(false);
    }
    else if (state == BOOT_CLIENT && client_state() == ON)
    {
        state = BOOT_DONE;
        (*boot_end)(true);
    }
    else if (state == BOOT_CLIENT && client_state() == FAIL)
    {
        state = BOOT_FAIL;
        server_cancel();
        boot_stop();
        (*boot_end)(false);
    }
}
