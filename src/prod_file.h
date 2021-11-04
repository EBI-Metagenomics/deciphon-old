#ifndef PROD_FILE_H
#define PROD_FILE_H

#include "path.h"
#include <stdio.h>

struct prod_file
{
    PATH_TEMP_DECLARE(path);
    FILE *fd;
};

enum dcp_rc prod_file_open(struct prod_file *file);
enum dcp_rc prod_file_close(struct prod_file *file);

#endif
