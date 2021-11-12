#include "dcp/srv.h"
#include "cco/cco.h"
#include "db_pool.h"
#include "dcp/db.h"
#include "dcp/generics.h"
#include "dcp/job.h"
#include "dcp/pro_codec.h"
#include "dcp/pro_state.h"
#include "dcp/prof_types.h"
#include "dcp/rc.h"
#include "error.h"
#include "fasta/fasta.h"
#include "gff/gff.h"
#include "macros.h"
#include "sched.h"
#include "sched_db.h"
#include "sched_job.h"
#include "table.h"
#include "tbl/tbl.h"
#include "work.h"
#include "xfile.h"
#include "xstrlcpy.h"

struct dcp_prod
{
    struct sched_prod sched_prod;
};

struct dcp_srv
{
    struct dcp_job job;
    struct dcp_prod prod;
};

struct dcp_srv *dcp_srv_open(char const *filepath)
{
    struct dcp_srv *srv = malloc(sizeof(*srv));
    if (!srv)
    {
        error(DCP_OUTOFMEM, "failed to malloc server");
        goto cleanup;
    }
    db_pool_module_init();

    enum dcp_rc rc = DCP_DONE;
    if ((rc = sched_setup(filepath))) goto cleanup;
    if ((rc = sched_open(filepath))) goto cleanup;

    return srv;

cleanup:
    free(srv);
    return NULL;
}

enum dcp_rc dcp_srv_close(struct dcp_srv *srv)
{
    enum dcp_rc rc = sched_close();
    free(srv);
    return rc;
}

enum dcp_rc dcp_srv_add_db(struct dcp_srv *srv, char const *name,
                           char const *filepath, int64_t *id)
{
    if (!xfile_is_readable(filepath))
        return error(DCP_IOERROR, "file is not readable");

    struct sched_db db = SCHED_DB_INIT();
    sched_db_setup(&db, name, filepath);

    enum dcp_rc rc = sched_db_add(&db);
    *id = db.id;
    return rc;
}

enum dcp_rc dcp_srv_submit_job(struct dcp_srv *srv, struct dcp_job *job)
{
    return sched_submit_job(job);
}

enum dcp_rc dcp_srv_job_state(struct dcp_srv *srv, int64_t job_id,
                              enum dcp_job_state *state)
{
    return sched_job_state(job_id, state);
}

static enum dcp_rc predict_codons(struct imm_seq const *seq,
                                  struct dcp_pro_prof *prof,
                                  struct imm_path const *path,
                                  char ocodon[5001 * 3])
{
    /* struct dcp_pro_prof *prof = &cli.pro.db.prof; */
    /* struct imm_path const *path = &cli.pro.prod.path; */

    struct dcp_pro_codec codec = dcp_pro_codec_init(prof, path);
    struct imm_codon codon = imm_codon_any(prof->nuclt);

    enum dcp_rc rc = DCP_DONE;
    /* char *ocodon = cli.output.codon.seq; */
    while (!(rc = dcp_pro_codec_next(&codec, seq, &codon)))
    {
        *(ocodon++) = imm_codon_asym(&codon);
        *(ocodon++) = imm_codon_bsym(&codon);
        *(ocodon++) = imm_codon_csym(&codon);
    }
    *ocodon = '\0';
    if (rc == DCP_END) rc = DCP_DONE;
    return rc;
}

static void decode_codons(char ocodon[5001 * 3], char oamino[5001],
                          struct imm_nuclt const *nuclt)
{
    struct imm_codon codon = imm_codon_any(nuclt);
    struct imm_abc const *abc = &codon.nuclt->super;
    while (*ocodon)
    {
        codon.a = imm_abc_symbol_idx(abc, *(ocodon++));
        codon.b = imm_abc_symbol_idx(abc, *(ocodon++));
        codon.c = imm_abc_symbol_idx(abc, *(ocodon++));
        *(oamino++) = imm_gc_decode(1, codon);
    }
    *oamino = '\0';
}

static void annotate(struct imm_seq const *sequence, char const *profile_name,
                     char const *seq_name, struct imm_path const *path,
                     char const ocodon[5001 * 3], char const oamino[5001 * 3],
                     struct dcp_pro_prof *prof)
{
    static struct tbl_8x_ed table = {0};
    char const *headers[4] = {profile_name, seq_name, "posterior", ""};
    tbl_8x_ed_setup(&table, stdout, 128, 6, 32, TBL_RIGHT, 4, headers);
    char const *seq = sequence->str;
    /* struct imm_path const *path = &cli.pro.prod.path; */

    /* char const *ocodon = cli.output.codon.seq; */
    /* char const *oamino = cli.output.amino.seq; */
    for (unsigned i = 0; i < imm_path_nsteps(path); ++i)
    {
        struct imm_step const *step = imm_path_step(path, i);

        bool is_match = dcp_pro_state_is_match(step->state_id);
        bool is_delete = dcp_pro_state_is_delete(step->state_id);
        char cons[2] = " ";
        /* cli.pro.db.prof.consensus[dcp_pro_state_idx(step->state_id)]; */
        if (is_match || is_delete)
            cons[0] = prof->consensus[dcp_pro_state_idx(step->state_id)];
        else if (dcp_pro_state_is_insert(step->state_id))
            cons[0] = '.';

        char codon[4] = {0};
        char amino[2] = {0};
        if (dcp_pro_state_is_mute(step->state_id))
        {
            codon[0] = ' ';
            codon[1] = ' ';
            codon[2] = ' ';
            amino[0] = ' ';
        }
        else
        {
            codon[0] = ocodon[0];
            codon[1] = ocodon[1];
            codon[2] = ocodon[2];
            amino[0] = *oamino;
            ocodon += 3;
            oamino++;
        }
        char str[6] = {0};
        memcpy(str, seq, step->seqlen);
        tbl_8x_ed_add_col(&table, TBL_LEFT,
                          (char const *[4]){cons, str, codon, amino});
        seq += step->seqlen;
    }

    tbl_8x_ed_flush(&table);
}

enum dcp_rc dcp_srv_run(struct dcp_srv *srv, bool blocking)
{
    struct work work = {0};
    work_init(&work);

    enum dcp_rc rc = work_next(&work);
    if (rc == DCP_NOTFOUND) return DCP_DONE;

    rc = work_run(&work);
    if (rc) return rc;
    return DCP_NEXT;
}

enum dcp_rc dcp_srv_next_prod(struct dcp_srv *srv, int64_t job_id,
                              int64_t *prod_id)
{
    return sched_prod_next(job_id, prod_id);
}
