#ifndef PRO_MATCH_H
#define PRO_MATCH_H

#include "dcp/rc.h"
#include "imm/imm.h"
#include <stdio.h>

struct pro_match
{
    char frag[5];
    char state[IMM_STATE_NAME_SIZE];
    char codon[4];
    char amino[2];
};

void pro_match_setup(struct pro_match *m, struct imm_seq frag,
                     char const state[static 1]);
void pro_match_set_codon(struct pro_match *m, struct imm_codon codon);
void pro_match_set_amino(struct pro_match *m, char amino);

/* Output example
 *             ___________________________
 *             |   match0   |   match1   |
 *             ---------------------------
 * Output----->| CG,M1,CGA,K;CG,M4,CGA,K |
 *             ---|-|---|--|--------------
 * -----------   /  |   |  \    ---------------
 * | matched |__/   |   |   \___| most likely |
 * | letters |      |   |       | amino acid  |
 * -----------      |   |       ---------------
 *      -------------   ---------------
 *      | hmm state |   | most likely |
 *      -------------   | codon       |
 *                      ---------------
 */

enum dcp_rc pro_match_write(struct pro_match *m, FILE *restrict fd);
enum dcp_rc pro_match_write_sep(FILE *restrict fd);

#endif
