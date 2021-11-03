#include "prod.h"
#include "dcp.h"
#include "dcp/strlcpy.h"
#include "error.h"

void prod_init(struct prod *p, unsigned match_id, char const seq_id[static 1],
               char const prof_id[static 1], unsigned start, unsigned end,
               char const abc[static 1], double loglik, double null_loglik,
               char const model[static 1], char const version[static 1],
               char const db_id[static 1], char const seq_hash[static 1])
{
    p->match_id = match_id;
    dcp_strlcpy(p->seq_id, seq_id, MEMBER_SIZE(*p, seq_id));
    dcp_strlcpy(p->prof_id, prof_id, MEMBER_SIZE(*p, prof_id));
    p->start = start;
    p->end = end;
    dcp_strlcpy(p->abc_id, abc, MEMBER_SIZE(*p, abc_id));
    p->loglik = loglik;
    p->null_loglik = p->null_loglik;
    dcp_strlcpy(p->model, model, MEMBER_SIZE(*p, model));
    dcp_strlcpy(p->version, version, MEMBER_SIZE(*p, version));
    dcp_strlcpy(p->db_id, db_id, MEMBER_SIZE(*p, db_id));
    dcp_strlcpy(p->seq_hash, seq_hash, MEMBER_SIZE(*p, seq_hash));
}

#ifdef TAB
#error "TAB should not have been defined"
#else
#define TAB "\t"
#endif

#define ERROR_WRITE error(DCP_IOERROR, "failed to write prod")

enum dcp_rc prod_write(struct prod *p, FILE *restrict fd)
{
    if (fprintf(fd, "%d" TAB, p->match_id) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->seq_id) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->prof_id) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%d" TAB "%d" TAB, p->start, p->end)) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->abc_id) < 0) return ERROR_WRITE;
    if (fprintf(fd, "%a" TAB, p->loglik)) return ERROR_WRITE;
    if (fprintf(fd, "%a" TAB, p->null_loglik)) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->model)) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->version)) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->db_id)) return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->seq_hash)) return ERROR_WRITE;
    return DCP_DONE;
}

#undef TAB
