#include "cli.h"
#include "logger.h"
#include "progress_file.h"
#include "protein_db.h"
#include "protein_reader.h"
#include "safe.h"
#include "xfile.h"
#include <argp.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

struct arguments
{
    char *args[1];
    int quiet;
    char const *input_file;
    char output_file[PATH_MAX];
};

static bool infer_output_file(struct arguments *args)
{
    size_t n = safe_strcpy(args->output_file, args->input_file, PATH_MAX);
    if (n >= PATH_MAX) return false;
    return xfile_set_path_ext(args->output_file, PATH_MAX, ".dcp");
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch (key)
    {
    case 'q':
        args->quiet = 1;
        break;

    case 'o':
        if (safe_strcpy(args->output_file, arg, PATH_MAX) >= PATH_MAX)
        {
            error(RC_ILLEGALARG, "output path is too long");
            return ENAMETOOLONG;
        }
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= 1) argp_usage(state);
        args->args[state->arg_num] = arg;
        args->input_file = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < 1)
        {
            argp_usage(state);
        }
        else
        {
            if (!args->output_file[0] && !infer_output_file(args))
            {
                error(RC_ILLEGALARG, "output path would be too long");
                return ENAMETOOLONG;
            }
        }
        break;

    case ARGP_KEY_FINI:
    case ARGP_KEY_SUCCESS:
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static char doc[] = "Press a HMMER3 file -- dcp-press file.hmm";
static char args_doc[] = "HMM";
static struct argp_option options[] = {
    {"quiet", 'q', 0, 0, "Disable output", 0},
    {"output", 'o', "FILE", 0, "Output to FILE instead of standard output", 0},
    {0}};
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

static struct
{
    struct
    {
        char const *file;
        FILE *fd;
    } input;
    struct
    {
        char const *file;
        FILE *fd;
    } output;
    struct progress_file progress;
    struct protein_db db;
    struct protein_reader reader;
} cli = {0};

static enum rc cli_setup(struct arguments const *args)
{
    cli_logger_setup();
    cli.input.file = args->input_file;
    cli.output.file = args->output_file;

    if (!(cli.input.fd = fopen(cli.input.file, "r")))
        return error(RC_IOERROR, "failed to open the hmm file");

    if (!(cli.output.fd = fopen(cli.output.file, "wb")))
        return error(RC_IOERROR, "failed to open the output file");

    progress_file_init(&cli.progress, cli.input.fd);

    cli.db = protein_db_default;

    enum rc rc =
        protein_db_openw(&cli.db, cli.output.fd, &imm_amino_iupac,
                         imm_super(&imm_dna_iupac), PROTEIN_CFG_DEFAULT);
    if (rc) return rc;

    protein_reader_init(&cli.reader, &cli.db.amino, &cli.db.code,
                        cli.db.prof.cfg, cli.input.fd);

    return RC_DONE;
}

static enum rc profile_write(void)
{
    profile_set_name(&cli.db.prof.super, protein_reader_meta(&cli.reader));

    enum rc rc = protein_profile_absorb(&cli.db.prof, &cli.reader.model);
    if (rc) return rc;

    return protein_db_write(&cli.db, &cli.db.prof);
}

enum rc cli_press(int argc, char **argv)
{
    struct arguments arguments = {0};
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return RC_ILLEGALARG;

    enum rc rc = cli_setup(&arguments);
    if (rc) goto cleanup;

    progress_file_start(&cli.progress, !arguments.quiet);
    while (!(rc = protein_reader_next(&cli.reader)))
    {
        rc = profile_write();
        progress_file_update(&cli.progress);
    }

    if (rc != RC_END)
    {
        error(rc, "failed to parse HMM file");
        goto cleanup;
    }

    rc = protein_db_close(&cli.db);

cleanup:
    progress_file_stop(&cli.progress);
    fclose(cli.input.fd);
    fclose(cli.output.fd);
    cli_logger_flush();
    return rc;
}
