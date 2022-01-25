#ifndef DB_TMP_H
#define DB_TMP_H

#include "cmp/cmp.h"
#include "common/rc.h"
#include <stdint.h>

struct db_tmp
{
    uint32_t prof_size;
    union
    {
        struct cmp_ctx_s cmps[3];
        struct
        {
            struct cmp_ctx_s prof_cmp;
            struct cmp_ctx_s mt_cmp;
            struct cmp_ctx_s dp_cmp;
        } __attribute__((packed));
    };
};

void db_tmp_setup(struct db_tmp *db);
enum rc db_tmp_init(struct db_tmp *db);
void db_tmp_close(struct db_tmp *db);

#endif
