#ifndef DECIPHON_CORE_URL_H
#define DECIPHON_CORE_URL_H

#include <stdbool.h>

enum
{
    URL_STEM_SIZE = 512,
    URL_QUERY_SIZE = 512,
};

struct url
{
    char full[URL_STEM_SIZE + URL_QUERY_SIZE];
    char *stem;
    char *query;
};

bool url_init(struct url *url, char const *stem);
void url_set_query(struct url *url, char const *query);

#endif
