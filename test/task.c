#include "dcp/dcp.h"
#include "hope/hope.h"

void test_task(void);

int main(void)
{
    test_task();
    return hope_status();
}

void test_task(void)
{
    struct dcp_task task;
    dcp_task_init(&task);
}
