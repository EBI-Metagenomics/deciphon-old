#include "cass/cass.h"
#include "dcp/dcp.h"

void test_create_destroy(void);
void test_start_destroy(void);
void test_start_stop_destroy(void);

int main(void)
{
    test_create_destroy();
    test_start_destroy();
    test_start_stop_destroy();
    return cass_status();
}

void test_create_destroy(void)
{
    char const*        filepath = "/Users/horta/tmp/pfam24.dcp";
    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_destroy(server), 0);
}

void test_start_destroy(void)
{
    char const* filepath = "/Users/horta/tmp/pfam24.dcp";

    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    cass_equal(dcp_server_destroy(server), 0);
}

void test_start_stop_destroy(void)
{
    char const* filepath = "/Users/horta/tmp/pfam24.dcp";

    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    dcp_server_stop(server);

    cass_equal(dcp_server_destroy(server), 0);
}
