#include "chdir.h"
#include "die.h"
#include <uv.h>

void chdir(char const *p)
{
    if (uv_chdir(p)) die();
}
