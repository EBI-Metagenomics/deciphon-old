#include "core/as.h"
#include "zc.h"

int as_int(char const *str) { return zc_strto_int(str, NULL, 10); }

long as_long(char const *str) { return zc_strto_long(str, NULL, 10); }

int32_t as_int32(char const *str) { return zc_strto_int32(str, NULL, 10); }

int64_t as_int64(char const *str) { return zc_strto_int64(str, NULL, 10); }
