#include "dcp/dcp.h"
#include "hope/hope.h"

void test_server_setup(void);
void test_server_reopen(void);

int main(void)
{
    test_server_setup();
    test_server_reopen();
    return hope_status();
}

void test_server_setup(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();
    remove(TMPDIR "/setup.sqlite3");
    EQ(dcp_server_setup(&srv, TMPDIR "/setup.sqlite3"), DCP_SUCCESS);
    dcp_server_close(&srv);
}

void test_server_reopen(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();
    remove(TMPDIR "/reopen.sqlite3");

    EQ(dcp_server_setup(&srv, TMPDIR "/reopen.sqlite3"), DCP_SUCCESS);
    dcp_server_close(&srv);

    EQ(dcp_server_setup(&srv, TMPDIR "/reopen.sqlite3"), DCP_SUCCESS);
    dcp_server_close(&srv);
}
