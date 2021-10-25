#include "dcp/dcp.h"
#include "hope/hope.h"
#include "std_db_examples.h"

void test_server_setup(void);
void test_server_reopen(void);
void test_server_add_std_db(void);

int main(void)
{
    test_server_setup();
    test_server_reopen();
    test_server_add_std_db();
    return hope_status();
}

void test_server_setup(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();
    remove(TMPDIR "/setup.sqlite3");
    EQ(dcp_server_setup(&srv, TMPDIR "/setup.sqlite3"), DCP_SUCCESS);
    EQ(dcp_server_close(&srv), DCP_SUCCESS);
}

void test_server_reopen(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();
    remove(TMPDIR "/reopen.sqlite3");

    EQ(dcp_server_setup(&srv, TMPDIR "/reopen.sqlite3"), DCP_SUCCESS);
    EQ(dcp_server_close(&srv), DCP_SUCCESS);

    EQ(dcp_server_setup(&srv, TMPDIR "/reopen.sqlite3"), DCP_SUCCESS);
    EQ(dcp_server_close(&srv), DCP_SUCCESS);
}

void test_server_add_std_db(void)
{
    struct dcp_server srv = DCP_SERVER_INIT();

    remove(TMPDIR "/add_std_db.sqlite3");
    EQ(dcp_server_setup(&srv, TMPDIR "/add_std_db.sqlite3"), DCP_SUCCESS);

    std_db_examples_new_ex1(TMPDIR "/example1.dcp");
    EQ(dcp_server_add_db(&srv, TMPDIR "/example1.dcp"), DCP_SUCCESS);
    /* /Users/horta/data/Pfam-A.5.dcp */

    EQ(dcp_server_close(&srv), DCP_SUCCESS);
}
