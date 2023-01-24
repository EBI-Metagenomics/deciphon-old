#include "seqlist.h"
#include "array_size_field.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "fs.h"
#include <limits.h>
#include <stdlib.h>

void seqlist_init(struct seqlist *x)
{
  json_init(x->json, array_size_field(struct seqlist, json));
  x->data = NULL;
  x->end = false;
  x->error = 0;
  x->size = 0;
}

static int jerr(int rc);

int seqlist_open(struct seqlist *x, char const *filepath)
{
  long size = 0;
  int rc = fs_readall(filepath, &size, (unsigned char **)&x->data);
  if (rc) return rc;

  if (size > INT_MAX) defer_return(DCP_ELARGEFILE);

  if ((rc = jerr(json_parse(x->json, (int)size, x->data)))) defer_return(rc);

  if (json_type(x->json) != JSON_ARRAY) defer_return(DCP_EJSONINVAL);

  x->size = json_nchild(x->json);
  if (x->size > 0)
  {
    json_reset(x->json);
    json_array_at(x->json, 0);
    if ((rc = jerr(json_error(x->json)))) defer_return(rc);
    x->scan_id = json_long_of(x->json, "scan_id");
  }
  else
    x->end = true;

  x->end = false;

  return rc;

defer:
  free(x->data);
  x->data = NULL;
  return 0;
}

long seqlist_scan_id(struct seqlist const *x) { return x->scan_id; }

void seqlist_rewind(struct seqlist *x)
{
  if (x->size == 0) return;
  json_reset(x->json);
  json_array_at(x->json, 0);
  x->end = false;
}

char const *seqlist_next(struct seqlist *x)
{
  int rc = 0;
  if (json_type(x->json) == JSON_SENTINEL)
  {
    x->end = true;
    return NULL;
  }

  char const *data = json_string_of(x->json, "data");

  json_right(x->json);
  if ((rc = jerr(json_error(x->json))))
  {
    x->error = rc;
    return NULL;
  }

  return data;
}

bool seqlist_end(struct seqlist const *x) { return x->end; }

int seqlist_error(struct seqlist const *x) { return x->error; }

int seqlist_size(struct seqlist const *x) { return x->size; }

void seqlist_close(struct seqlist *x)
{
  if (x->data) free(x->data);
  x->data = NULL;
}

static int jerr(int rc)
{
  if (rc == JSON_INVAL) return DCP_EJSONINVAL;
  if (rc == JSON_NOMEM) return DCP_ENOMEM;
  if (rc == JSON_OUTRANGE) return DCP_EJSONRANGE;
  if (rc == JSON_NOTFOUND) return DCP_EJSONFOUND;
  return 0;
}
