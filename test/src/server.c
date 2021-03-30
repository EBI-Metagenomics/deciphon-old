#include "cass/cass.h"
#include "dcp/dcp.h"

void test_create_destroy(void);
void test_create_start_destroy(void);

int main(void)
{
    test_create_destroy();
    test_create_start_destroy();
    return cass_status();
}

void test_create_destroy(void)
{
    char const*        filepath = "/Users/horta/tmp/pfam24.dcp";
    struct dcp_server* server = dcp_server_create(filepath);
    dcp_server_destroy(server);
}

void test_create_start_destroy(void)
{
    char const*        filepath = "/Users/horta/tmp/pfam24.dcp";
    struct dcp_server* server = dcp_server_create(filepath);
    dcp_server_start(server);
    dcp_server_destroy(server);
}
