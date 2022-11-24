#include "scan/prodfile_reader.h"
#include "bzero.h"
#include "itoa.h"
#include "logy.h"
#include "rc.h"
#include "repr_size.h"
#include "scan/prod.h"
#include "scan/prodfile_header.h"
#include "sizeof_field.h"
#include "to.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define PREMATCH_NFIELDS 8

#define PREMATCH_SIZE                                                          \
    repr_size_field(struct prod, scan_id) +                                    \
        repr_size_field(struct prod, seq_id) +                                 \
        sizeof_field(struct prod, profile_name) +                              \
        sizeof_field(struct prod, abc_name) +                                  \
        repr_size_field(struct prod, alt_loglik) +                             \
        repr_size_field(struct prod, null_loglik) +                            \
        sizeof_field(struct prod, profile_typeid) +                            \
        sizeof_field(struct prod, version) + PREMATCH_NFIELDS

#define MATCH_SIZE sizeof_field(struct prod, match)

struct prodfile_reader
{
    FILE *fp;
    char header[sizeof(PRODFILE_HEADER) + 1];
    char buf[PREMATCH_SIZE + MATCH_SIZE + 1];
};

int prodfile_reader_new(struct prodfile_reader **ptr)
{
    *ptr = malloc(sizeof(struct prodfile_reader));
    if (!*ptr) return enomem("could not allocate for prodfile_reader");

    (*ptr)->fp = NULL;
    bzero((*ptr)->header, sizeof_field(struct prodfile_reader, header));

    return 0;
}

int prodfile_reader_open(struct prodfile_reader *r, char const *file)
{
    if (!(r->fp = fopen(file, "r"))) return eio("could not open %s", file);

    if (!fgets(r->header, sizeof_field(struct prodfile_reader, header), r->fp))
    {
        if (feof(r->fp))
        {
            fclose(r->fp);
            r->fp = NULL;
            return eio("unexpected end of file");
        }
        fclose(r->fp);
        r->fp = NULL;
        return eio("could not read prodfile header");
    }

    r->header[strlen(r->header) - 1] = '\0';

    if (strcmp(r->header, PRODFILE_HEADER))
        return einval("invalid prodfile header");

    return 0;
}

enum state
{
    STATE_SCAN_ID,
    STATE_SEQ_ID,
    STATE_PROFILE_NAME,
    STATE_ABC_NAME,
    STATE_ALT_LOGLIK,
    STATE_NULL_LOGLIK,
    STATE_PROFILE_TYPEID,
    STATE_VERSION,
    STATE_MATCH,
};

#define sizeof_prod(MEMBER) sizeof_field(struct prod, MEMBER)
#define parse_fail(FIELD) eparse("could not parse " FIELD)

static int parse_next(struct prod *p, char *str, enum state state);

int prodfile_reader_next(struct prodfile_reader *r, struct prod *p)
{
    size_t n = sizeof_field(struct prodfile_reader, buf);

    if (!fgets(r->buf, n, r->fp))
        return feof(r->fp) ? RC_END : eio("could not read next file line");

    char *ctx = NULL;
    enum state state = STATE_SCAN_ID;
    char *tok = strtok_r(r->buf, "\t\n", &ctx);
    while (tok)
    {
        int rc = parse_next(p, tok, state);
        if (rc) return rc;

        state += 1;
        tok = strtok_r(NULL, "\t\n", &ctx);
    }

    return 0;
}

int prodfile_reader_close(struct prodfile_reader *r)
{
    return fclose(r->fp) ? RC_EIO : 0;
}

void prodfile_reader_del(struct prodfile_reader *r) { free(r); }

static bool cp(char *dst, char const *src, size_t n)
{
    return strlcpy(dst, src, n) < n;
}

static int parse_next(struct prod *p, char *str, enum state state)
{
    size_t n = 0;
    switch (state)
    {
    case STATE_SCAN_ID:
        if (!to_long(str, &p->scan_id)) return parse_fail("scan_id");
        break;
    case STATE_SEQ_ID:
        if (!to_long(str, &p->seq_id)) return parse_fail("seq_id");
        break;
    case STATE_PROFILE_NAME:
        n = sizeof_prod(profile_name);
        if (!cp(p->profile_name, str, n)) return parse_fail("profile_name");
        break;
    case STATE_ABC_NAME:
        n = sizeof_prod(abc_name);
        if (!cp(p->abc_name, str, n)) return parse_fail("abc_name");
        break;
    case STATE_ALT_LOGLIK:
        if (!to_double(str, &p->alt_loglik)) return parse_fail("alt_loglik");
        break;
    case STATE_NULL_LOGLIK:
        if (!to_double(str, &p->null_loglik)) return parse_fail("null_loglik");
        break;
    case STATE_PROFILE_TYPEID:
        n = sizeof_prod(profile_typeid);
        if (!cp(p->profile_typeid, str, n)) return parse_fail("profile_typeid");
        break;
    case STATE_VERSION:
        n = sizeof_prod(version);
        if (!cp(p->version, str, n)) return parse_fail("version");
        break;
    case STATE_MATCH:
        n = sizeof_prod(match);
        if (!cp(p->match, str, n)) return parse_fail("match");
        break;
    default:
        return eparse("unrecognized parsing state");
    }
    return 0;
}
