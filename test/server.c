#include "dcp/dcp.h"
#include "hope/hope.h"

void test_server_init(void);
void test_server_init_with_host(void);
/* void test_server_reopen_database(void); */

int main(void)
{
    test_server_init();
    test_server_init_with_host();
    /* test_server_reopen_database(); */
    return hope_status();
}

void test_server_init(void)
{
    struct dcp_server srv;
    EQ(dcp_server_init(&srv, "file:" TMPDIR "/init.sqlite3"), DCP_SUCCESS);
    dcp_server_close(&srv);
}

void test_server_init_with_host(void)
{
    struct dcp_server srv;
    EQ(dcp_server_init(&srv, "file://" TMPDIR "/init.sqlite3"), DCP_ILLEGALARG);
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
