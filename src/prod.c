#include "prod.h"
#include "dcp.h"
#include "dcp/strlcpy.h"
#include "error.h"

void prod_init(struct prod *p, unsigned match_id, char const seq_id[static 1],
               char const prof_id[static 1], char const abc[static 1],
               unsigned start, unsigned end, double loglik, double null_loglik,
               char const model[static 1])
{
    p->match_id = match_id;
    dcp_strlcpy(p->seq_id, seq_id, MEMBER_SIZE(*p, seq_id));
    dcp_strlcpy(p->prof_id, prof_id, MEMBER_SIZE(*p, prof_id));
    dcp_strlcpy(p->abc, abc, MEMBER_SIZE(*p, abc));
    p->end = end;
    p->loglik = loglik;
    p->null_loglik = p->null_loglik;
    dcp_strlcpy(p->model, model, MEMBER_SIZE(*p, model));
}

#ifdef TAB
#error "TAB should not have been defined"
#else
#define TAB "\t"
#endif

#define ERROR_WRITE error(DCP_IOERROR, "failed to write prod")

/*
 * Column: match_id, seq_id, prof_id, abc, start, end,
 *         loglik, null_loglik, model, and match.
 */
enum dcp_rc prod_write(struct prod *p, FILE *restrict fd)
{
    if (fprintf(fd, "%d" TAB "%s" TAB, p->match_id, p->seq_id) < 0)
        return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB "%s" TAB, p->prof_id, p->abc)) return ERROR_WRITE;
    if (fprintf(fd, "%d" TAB "%d" TAB, p->start, p->end)) return ERROR_WRITE;
    if (fprintf(fd, "%a" TAB "%a" TAB, p->loglik, p->null_loglik))
        return ERROR_WRITE;
    if (fprintf(fd, "%s" TAB, p->model)) return ERROR_WRITE;

    return DCP_DONE;
}

#undef TAB
