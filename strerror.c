#include "deciphon/strerror.h"

#if 0
static const struct {
	int         code;
	const char *msg;
} nni_errors[] = {
	// clang-format off
	{ 0, "not an error" },
	{ NNG_EINTR, "Interrupted" },
	{ 0, NULL },
	// clang-format on
};
#endif

char const *dcp_strerror(int errno) { return ""; }
