#ifndef DB_H
#define DB_H

#include "dcp/db.h"
#include "dcp/prof.h"

void db_init(struct dcp_db *db, enum dcp_prof_typeid prof_typeid);

void db_openr(struct dcp_db *db, FILE *restrict fd);
enum dcp_rc db_openw(struct dcp_db *db, FILE *restrict fd);
enum dcp_rc db_close(struct dcp_db *db);
bool db_end(struct dcp_db const *db);

enum dcp_rc db_read_magic_number(struct dcp_db *db);
enum dcp_rc db_write_magic_number(struct dcp_db *db);

enum dcp_rc db_read_prof_type(struct dcp_db *db);
enum dcp_rc db_write_prof_type(struct dcp_db *db);

enum dcp_rc db_read_float_size(struct dcp_db *db);
enum dcp_rc db_write_float_size(struct dcp_db *db);

enum dcp_rc db_read_nprofiles(struct dcp_db *db);
enum dcp_rc db_read_metadata(struct dcp_db *db);

enum dcp_rc db_write_prof_meta(struct dcp_db *db, struct dcp_prof const *prof);

enum dcp_rc db_check_write_prof_ready(struct dcp_db const *db,
                                      struct dcp_prof const *prof);

struct dcp_meta db_meta(struct dcp_db const *db, unsigned idx);

#endif
