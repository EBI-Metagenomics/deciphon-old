#include "dcp/dcp.h"
#include "hope/hope.h"

void test_server_init(void);

int main(void)
{
    test_server_init();
    return hope_status();
}

void test_server_init(void)
{
    struct dcp_server srv;
    EQ(dcp_server_init(&srv, TMPDIR "/deciphon.sqlite3"), DCP_SUCCESS);

    dcp_server_close(&srv);
}
