#include "cli.h"
#include "deciphon/db/db.h"
#include "deciphon/model/model.h"
#include "deciphon/rc.h"
#include "deciphon/util/util.h"
#include "progress_file.h"
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
    size_t n = strlcpy(args->output_file, args->input_file, PATH_MAX);
    if (n >= PATH_MAX) return false;
    return !xfile_set_ext(PATH_MAX, args->output_file, ".dcp");
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
        if (strlcpy(args->output_file, arg, PATH_MAX) >= PATH_MAX)
        {
            error(RC_EINVAL, "output path is too long");
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
                error(RC_EINVAL, "output path would be too long");
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
    struct protein_db_writer db;
    struct protein_profile profile;
    struct protein_h3reader reader;
} cli = {0};

static enum rc cli_setup(struct arguments const *args)
{
    cli_logger_setup();
    cli.input.file = args->input_file;
    cli.output.file = args->output_file;

    if (!(cli.input.fd = fopen(cli.input.file, "r")))
        return error(RC_EIO, "failed to open the hmm file");

    if (!(cli.output.fd = fopen(cli.output.file, "wb")))
        return error(RC_EIO, "failed to open the output file");

    progress_file_init(&cli.progress, cli.input.fd);

    enum rc rc =
        protein_db_writer_open(&cli.db, cli.output.fd, &imm_amino_iupac,
                               imm_super(&imm_dna_iupac), PROTEIN_CFG_DEFAULT);
    if (rc) return rc;

    protein_h3reader_init(&cli.reader, &cli.db.amino, &cli.db.code, cli.db.cfg,
                          cli.input.fd);

    protein_profile_init(&cli.profile, cli.reader.prof.meta.acc, &cli.db.amino,
                         &cli.db.code, cli.db.cfg);

    return RC_OK;
}

static enum rc profile_write(void)
{
    enum rc rc = protein_profile_absorb(&cli.profile, &cli.reader.model);
    if (rc) return rc;

    strlcpy(cli.profile.super.accession, cli.reader.prof.meta.acc,
            ARRAY_SIZE_OF(cli.profile.super, accession));
    return protein_db_writer_pack_profile(&cli.db, &cli.profile);
}

enum rc cli_press(int argc, char **argv)
{
    struct arguments arguments = {0};
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return RC_EINVAL;

    enum rc rc = cli_setup(&arguments);
    if (rc) goto cleanup;

    progress_file_start(&cli.progress, !arguments.quiet);
    while (!(rc = protein_h3reader_next(&cli.reader)))
    {
        if ((rc = profile_write()))
        {
            efail("failed to write profile");
            goto cleanup;
        }
        progress_file_update(&cli.progress);
    }

    if (rc != RC_END)
    {
        error(rc, "failed to parse HMM file");
        goto cleanup;
    }

    rc = db_writer_close((struct db_writer *)&cli.db, true);

cleanup:
    progress_file_stop(&cli.progress);
    fclose(cli.input.fd);
    fclose(cli.output.fd);
    cli_logger_flush();
    return rc;
}
