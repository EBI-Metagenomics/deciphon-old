#include "dcp/dcp.h"
#include "hope/hope.h"

void test_server_setup(void);
/* void test_server_reopen_database(void); */

int main(void)
{
    test_server_setup();
    /* test_server_reopen_database(); */
    return hope_status();
}

void test_server_setup(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();
    EQ(dcp_server_setup(&srv, TMPDIR "/init.sqlite3"), DCP_SUCCESS);
    dcp_server_close(&srv);
}
#if 0
void test_server_reopen_database(void)
{
    struct dcp_server srv;
    EQ(dcp_server_init(&srv, "file://" TMPDIR "/reopen.sqlite3"), DCP_SUCCESS);
    dcp_server_close(&srv);

    EQ(dcp_server_init(&srv, "file://" TMPDIR "/reopen.sqlite3"), DCP_SUCCESS);
    dcp_server_close(&srv);
}
#endif
