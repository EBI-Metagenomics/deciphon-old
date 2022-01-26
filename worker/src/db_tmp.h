#ifndef DB_TMP_H
#define DB_TMP_H

#include "cmp/cmp.h"
#include "common/rc.h"
#include <stdint.h>

struct db_tmp
{
    union
    {
        struct cmp_ctx_s cmps[4];
        struct
        {
            struct cmp_ctx_s hdr;
            struct cmp_ctx_s mt;
            struct cmp_ctx_s size;
            struct cmp_ctx_s prof;
        } __attribute__((packed));
    };
};

void db_tmp_setup(struct db_tmp *db);
enum rc db_tmp_init(struct db_tmp *db);
void db_tmp_close(struct db_tmp *db);

#endif
