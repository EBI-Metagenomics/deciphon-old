#include "seq_list.h"
#include "array_size_field.h"
#include "deciphon/errno.h"
#include "defer_return.h"
#include "fs.h"
#include "strlcpy.h"
#include <limits.h>
#include <stdlib.h>

void seq_list_init(struct seq_list *x)
{
  json_init(x->json, array_size_field(struct seq_list, json));
  x->data = NULL;
  x->end = false;
  x->size = 0;
}

static int jerr(int rc);

int seq_list_open(struct seq_list *x)
{
  long size = 0;
  int rc = fs_readall(x->filename, &size, (unsigned char **)&x->data);
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

void seq_list_close(struct seq_list *x)
{
  if (x->data) free(x->data);
  x->data = NULL;
}

int seq_list_set_filename(struct seq_list *x, char const *filename)
{
  size_t n = array_size_field(struct seq_list, filename);
  return strlcpy(x->filename, filename, n) < n ? 0 : DCP_ELONGPATH;
}

long seq_list_scan_id(struct seq_list const *x) { return x->scan_id; }

void seq_list_rewind(struct seq_list *x)
{
  if (x->size == 0) return;
  json_reset(x->json);
  json_array_at(x->json, 0);
  x->end = false;
}

int seq_list_next(struct seq_list *x)
{
  if (json_type(x->json) == JSON_SENTINEL)
  {
    x->end = true;
    return 0;
  }

  x->seq_id = json_long_of(x->json, "id");
  x->seq_name = json_string_of(x->json, "name");
  x->seq_data = json_string_of(x->json, "data");

  json_right(x->json);

  return jerr(json_error(x->json));
}

bool seq_list_end(struct seq_list const *x) { return x->end; }

int seq_list_size(struct seq_list const *x) { return x->size; }

struct seq seq_list_get(struct seq_list const *x, struct imm_abc const *abc)
{
  return seq_init(x->seq_id, x->seq_name, x->seq_data, abc);
}

static int jerr(int rc)
{
  if (rc == JSON_INVAL) return DCP_EJSONINVAL;
  if (rc == JSON_NOMEM) return DCP_ENOMEM;
  if (rc == JSON_OUTRANGE) return DCP_EJSONRANGE;
  if (rc == JSON_NOTFOUND) return DCP_EJSONFOUND;
  return 0;
}
