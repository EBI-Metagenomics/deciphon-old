#include "tbl/tbl.h"

void annotate(struct imm_seq const *sequence, char const *profile_name,
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
