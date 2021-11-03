#ifndef PRO_MATCH_H
#define PRO_MATCH_H

#include "cco/cco.h"
#include "dcp/rc.h"
#include <stdio.h>

struct pro_match
{
    char frag[5];
    char state[6];
    char codon[4];
    char amino;
    struct cco_node node;
};

void pro_match_init(struct pro_match *m, char const frag[static 1],
                    char const state[static 1], char const codon[static 1],
                    char amino);

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
