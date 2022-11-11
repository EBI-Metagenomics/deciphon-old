#include "boot.h"
#include "client.h"
#include "core/global.h"
#include "server.h"
#include "uv.h"

enum boot_state
{
    BOOT_INIT,
    BOOT_SERVER,
    BOOT_CLIENT,
    BOOT_DONE,
    BOOT_FAIL,
};

static boot_end_fn_t *boot_end = NULL;
static uv_timer_t timer = {0};
static enum boot_state state = BOOT_INIT;

void boot_init(void) { uv_timer_init(global_loop(), &timer); }

static void callback(struct uv_timer_s *req);

void boot_start(char const *hmm_file, boot_end_fn_t *boot_end_fn)
{
    if (state != BOOT_INIT && state != BOOT_FAIL) boot_stop();
    boot_end = boot_end_fn;
    uv_timer_start(&timer, &callback, 1000, 100);
    state = BOOT_SERVER;
    server_start(hmm_file);
}

void boot_stop(void)
{
    uv_timer_stop(&timer);
    if (state == BOOT_SERVER) server_stop();
    if (state == BOOT_CLIENT) client_stop();
}

bool boot_offline(void) { return state == BOOT_INIT || state == BOOT_FAIL; }

void boot_cleanup(void)
{
    uv_timer_stop(&timer);
    uv_close((struct uv_handle_s *)&timer, NULL);
    state = BOOT_INIT;
}

static void callback(struct uv_timer_s *req)
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
        server_stop();
        boot_stop();
        (*boot_end)(false);
    }
}
