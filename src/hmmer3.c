#include "easel.h"
#include "esl_alphabet.h"
#include "esl_getopts.h"
#include "esl_sq.h"
#include "esl_sqio.h"
#include "esl_threads.h"
#include "esl_workqueue.h"
#include "hmmer.h"
#include "p7_config.h"
#include <unistd.h>

struct cfg_s
{
    char *seqfile; /* query sequence file                           */
    char *hmmfile; /* database HMM file                             */
    int do_mpi;    /* TRUE if we're doing MPI parallelization       */
    int nproc;     /* how many MPI processes, total                 */
    int my_rank;   /* who am I, in 0..nproc-1                       */
};

typedef struct
{
    ESL_SQ *qsq;
    P7_BG *bg;        /* null model                              */
    P7_PIPELINE *pli; /* work pipeline                           */
    P7_TOPHITS *th;   /* top hit results                         */
} WORKER_INFO;

int main(int argc, char **argv)
{
    struct cfg_s cfg;
    cfg.hmmfile = argv[1];
    cfg.seqfile = argv[2];
    cfg.do_mpi = FALSE;             /* this gets reset below, if we init MPI */
    cfg.nproc = 0;                  /* this gets reset below, if we init MPI */
    int seqfmt = eslSQFILE_UNKNOWN; /* format of seqfile */
    ESL_SQFILE *sqfp = NULL;        /* open seqfile        */
    cfg.my_rank = 0;                /* this gets reset below, if we init MPI */
    char errbuf[eslERRBUFSIZE];
    P7_HMMFILE *hfp = NULL; /* open HMM database file */
    int hstatus = eslOK;
    int status = eslOK;
    P7_OPROFILE *om = NULL;   /* target profile */
    ESL_ALPHABET *abc = NULL; /* sequence alphabet */
    ESL_SQ *qsq = NULL; /* query sequence                                  */
    P7_OM_BLOCK *block = NULL;
    ESL_THREADS *threadObj = NULL;
    ESL_WORK_QUEUE *queue = NULL;
    ESL_STOPWATCH *w = esl_stopwatch_Create();

    /* Open the target profile database to get the sequence alphabet */
    status = p7_hmmfile_OpenE(cfg.hmmfile, p7_HMMDBENV, &hfp, errbuf);
    if (status == eslENOTFOUND)
        printf("File existence/permissions problem in trying to open HMM file "
               "%s.\n%s\n",
               cfg.hmmfile, errbuf);
    else if (status == eslEFORMAT)
        printf("File format problem, trying to open HMM file %s.\n%s\n",
               cfg.hmmfile, errbuf);
    else if (status != eslOK)
        printf("Unexpected error %d in opening HMM file %s.\n%s\n", status,
               cfg.hmmfile, errbuf);
    if (!hfp->is_pressed)
        printf("Failed to open binary auxfiles for %s: use hmmpress first\n",
               hfp->fname);

    hstatus = p7_oprofile_ReadMSV(hfp, &abc, &om);
    if (hstatus == eslEFORMAT)
        p7_Fail("bad format, binary auxfiles, %s:\n%s", cfg.hmmfile,
                hfp->errbuf);
    else if (hstatus == eslEINCOMPAT)
        p7_Fail("HMM file %s contains different alphabets", cfg.hmmfile);
    else if (hstatus != eslOK)
        p7_Fail("Unexpected error in reading HMMs from %s", cfg.hmmfile);

    p7_oprofile_Destroy(om);
    p7_hmmfile_Close(hfp);

    /* Open the query sequence database */
    status = esl_sqfile_OpenDigital(abc, cfg.seqfile, seqfmt, NULL, &sqfp);
    if (status == eslENOTFOUND)
        p7_Fail("Failed to open sequence file %s for reading\n", cfg.seqfile);
    else if (status == eslEFORMAT)
        p7_Fail("Sequence file %s is empty or misformatted\n", cfg.seqfile);
    else if (status == eslEINVAL)
        p7_Fail("Can't autodetect format of a stdin or .gz seqfile");
    else if (status != eslOK)
        p7_Fail("Unexpected error %d opening sequence file %s\n", status,
                cfg.seqfile);
    qsq = esl_sq_CreateDigital(abc);

    int infocnt = 1;
    int i = 0;
    WORKER_INFO *info = NULL;
    ESL_ALLOC(info, sizeof(*info) * infocnt);

    for (i = 0; i < infocnt; ++i)
    {
        info[i].bg = p7_bg_Create(abc);
    }

    int sstatus = eslOK;
    int nquery = 0;
    FILE *ofp = stdout; /* output file for results (default stdout)        */

    ESL_GETOPTS go;
    go.nopts = 3;
    go.opt = malloc(sizeof(ESL_OPTIONS) * go.nopts);
    ESL_OPTIONS *opt = (ESL_OPTIONS *)go.opt;
    opt[0].name = "--seed";
    opt[0].type = eslARG_INT;
    opt[1].name = "-E";
    opt[1].type = eslARG_REAL;
    opt[1].defval = "10.0";
    opt[2].name = "--domE";
    opt[2].type = eslARG_REAL;
    opt[2].defval = "10.0";
    opt[3].name = "-T";
    opt[3].type = eslARG_REAL;
    opt[3].defval = FALSE;

    /* Outside loop: over each query sequence in <seqfile>. */
    while ((sstatus = esl_sqio_Read(sqfp, qsq)) == eslOK)
    {
        nquery++;
        esl_stopwatch_Start(w);

        /* Open the target profile database */
        status = p7_hmmfile_OpenE(cfg.hmmfile, p7_HMMDBENV, &hfp, NULL);
        if (status != eslOK)
            p7_Fail("Unexpected error %d in opening hmm file %s.\n", status,
                    cfg.hmmfile);

        if (fprintf(ofp, "Query:       %s  [L=%ld]\n", qsq->name,
                    (long)qsq->n) < 0)
            ESL_EXCEPTION_SYS(eslEWRITE, "write failed");
        if (qsq->acc[0] != 0 && fprintf(ofp, "Accession:   %s\n", qsq->acc) < 0)
            ESL_EXCEPTION_SYS(eslEWRITE, "write failed");
        if (qsq->desc[0] != 0 &&
            fprintf(ofp, "Description: %s\n", qsq->desc) < 0)
            ESL_EXCEPTION_SYS(eslEWRITE, "write failed");

        for (i = 0; i < infocnt; ++i)
        {
            /* Create processing pipeline and hit list */
            info[i].th = p7_tophits_Create();
            info[i].pli = p7_pipeline_Create(
                &go, 100, 100, FALSE,
                p7_SCAN_MODELS); /* M_hint = 100, L_hint = 100 are just dummies
                                    for now */
            info[i].pli->hfp =
                hfp; /* for two-stage input, pipeline needs <hfp> */

            p7_pli_NewSeq(info[i].pli, qsq);
            info[i].qsq = qsq;
        }

        /* hstatus = serial_loop(info, hfp); */
        switch (hstatus)
        {
        case eslEFORMAT:
            p7_Fail("bad file format in HMM file %s", cfg.hmmfile);
            break;
        case eslEINCOMPAT:
            p7_Fail("HMM file %s contains different alphabets", cfg.hmmfile);
            break;
        case eslEOF: /* do nothing */
            break;
        default:
            p7_Fail("Unexpected error in reading HMMs from %s", cfg.hmmfile);
        }

        /* merge the results of the search results */
        for (i = 1; i < infocnt; ++i)
        {
            p7_tophits_Merge(info[0].th, info[i].th);
            p7_pipeline_Merge(info[0].pli, info[i].pli);

            p7_pipeline_Destroy(info[i].pli);
            p7_tophits_Destroy(info[i].th);
        }

        /* Print results */
        p7_tophits_SortBySortkey(info->th);
        p7_tophits_Threshold(info->th, info->pli);

        int textw = 0;
        p7_tophits_Targets(ofp, info->th, info->pli, textw);
        if (fprintf(ofp, "\n\n") < 0)
            ESL_EXCEPTION_SYS(eslEWRITE, "write failed");
        p7_tophits_Domains(ofp, info->th, info->pli, textw);
        if (fprintf(ofp, "\n\n") < 0)
            ESL_EXCEPTION_SYS(eslEWRITE, "write failed");

#if 0
        if (tblfp)
            p7_tophits_TabularTargets(tblfp, qsq->name, qsq->acc, info->th,
                                      info->pli, (nquery == 1));
        if (domtblfp)
            p7_tophits_TabularDomains(domtblfp, qsq->name, qsq->acc, info->th,
                                      info->pli, (nquery == 1));
        if (pfamtblfp)
            p7_tophits_TabularXfam(pfamtblfp, qsq->name, qsq->acc, info->th,
                                   info->pli);
#endif

        esl_stopwatch_Stop(w);
        p7_pli_Statistics(ofp, info->pli, w);
        if (fprintf(ofp, "//\n") < 0)
            ESL_EXCEPTION_SYS(eslEWRITE, "write failed");
        fflush(ofp);

        p7_hmmfile_Close(hfp);
        p7_pipeline_Destroy(info->pli);
        p7_tophits_Destroy(info->th);
        esl_sq_Reuse(qsq);
    }

    if (sstatus == eslEFORMAT)
        esl_fatal("Parse failed (sequence file %s):\n%s\n", sqfp->filename,
                  esl_sqfile_GetErrorBuf(sqfp));
    else if (sstatus != eslEOF)
        esl_fatal("Unexpected error %d reading sequence file %s", sstatus,
                  sqfp->filename);

        /* Terminate outputs - any last words?
         */
#if 0
    if (tblfp)
        p7_tophits_TabularTail(tblfp, "hmmscan", p7_SCAN_MODELS, cfg->seqfile,
                               cfg->hmmfile, go);
    if (domtblfp)
        p7_tophits_TabularTail(domtblfp, "hmmscan", p7_SCAN_MODELS,
                               cfg->seqfile, cfg->hmmfile, go);
    if (pfamtblfp)
        p7_tophits_TabularTail(pfamtblfp, "hmmscan", p7_SEARCH_SEQS,
                               cfg->seqfile, cfg->hmmfile, go);
#endif
    if (ofp)
    {
        if (fprintf(ofp, "[ok]\n") < 0)
            ESL_EXCEPTION_SYS(eslEWRITE, "write failed");
    }

    /* Cleanup - prepare for successful exit
     */
    for (i = 0; i < infocnt; ++i)
        p7_bg_Destroy(info[i].bg);

    free(info);

    esl_sq_Destroy(qsq);
    esl_stopwatch_Destroy(w);
    esl_alphabet_Destroy(abc);
    esl_sqfile_Close(sqfp);

    return eslOK;

ERROR:
    return status;
}
