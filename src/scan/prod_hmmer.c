#include "scan/prod_hmmer.h"
#include "h3c/h3c.h"
#include "itoa.h"
#include "logy.h"
#include "sizeof_field.h"
#include "strlcpy.h"
#include <stdio.h>
#include <string.h>

#if 0
void prod_hmmer_init(struct prod_hmmer *x)
{
    x->scan_id = 0; x->seq_id = 0;
    x->profile_name[0] = '\0';
    x->filename[0] = '\0';
}

void prod_hmmer_set_scan_id(struct prod_hmmer *x, long scan_id)
{
    x->scan_id = scan_id;
}

void prod_hmmer_set_seq_id(struct prod_hmmer *x, long seq_id)
{
    x->seq_id = seq_id;
}

int prod_hmmer_set_profname(struct prod_hmmer *x, char const *profname)
{
    size_t n = sizeof_field(struct prod_hmmer, profile_name);
    if (strlcpy(x->profile_name, profname, n) >= n)
        return einval("too long path");
    return 0;
}

int prod_hmmer_write(struct prod_hmmer *x, struct h3c_result *r)
{
    FILE *fp = fopen(prod_hmmer_filename(x), "wb");
    if (!fp) return eio("could not open file %s", prod_hmmer_filename(x));

    if (h3c_result_pack(r, fp))
    {
        fclose(fp);
        return eio("could not write hmmer results");
    }

    return fclose(fp) ? eio("could not close file") : 0;
}

int prod_hmmer_read(struct prod_hmmer *x, struct h3c_result *r)
{
    FILE *fp = fopen(prod_hmmer_filename(x), "rb");
    if (!fp) return eio("could not open file %s", prod_hmmer_filename(x));

    if (h3c_result_unpack(r, fp))
    {
        fclose(fp);
        return eio("could not read hmmer results");
    }

    return fclose(fp) ? eio("could not close file") : 0;
}
#endif

char const *prod_hmmer_filename(char *filename, long scan_id, long seq_id,
                                char const *profile_name)
{
    char *p = filename;

    p += strlen(strcpy(p, "hmmer"));

    p += strlen(strcpy(p, "_"));
    p += ltoa(p, scan_id);

    p += strlen(strcpy(p, "_"));
    p += ltoa(p, seq_id);

    p += strlen(strcpy(p, "_"));
    p += strlen(strcpy(p, profile_name));

    p += strlen(strcat(p, ".h3r"));
    *p = '\0';

    return filename;
}
