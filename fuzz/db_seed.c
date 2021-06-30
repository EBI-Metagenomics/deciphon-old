#include "imm/imm.h"

int main(void)
{
    struct imm_abc abc_empty = imm_abc_empty;
    struct imm_abc *abc = &abc_empty;
    imm_abc_init(abc, IMM_STR("ACGT"), '*');
    FILE *file = fopen(CORPUS "/abc_acgt.imm", "wb");
    int err = imm_abc_write(abc, file);
    fclose(file);
    if (err)
        return err;

    struct imm_dna dna = imm_dna_default;
    file = fopen(CORPUS "/dna.imm", "wb");
    err = imm_abc_write(imm_super(imm_super(&dna)), file);
    fclose(file);
    if (err)
        return err;

    struct imm_rna rna = imm_rna_default;
    file = fopen(CORPUS "/rna.imm", "wb");
    err = imm_abc_write(imm_super(imm_super(&rna)), file);
    fclose(file);
    if (err)
        return err;

    struct imm_amino amino = imm_amino_default;
    file = fopen(CORPUS "/amino.imm", "wb");
    err = imm_abc_write(imm_super(&amino), file);
    fclose(file);
    if (err)
        return err;

    return err;
}
