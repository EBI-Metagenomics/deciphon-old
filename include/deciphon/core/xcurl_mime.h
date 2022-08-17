#ifndef DECIPHON_CORE_XCURL_MIME_H
#define DECIPHON_CORE_XCURL_MIME_H

struct xcurl_mime_file
{
    char const *name;
    char const *filename;
    char const *type;
};

#define XCURL_MIME_FILE_DEF(var, name, filename, type)                         \
    struct xcurl_mime_file var = (struct xcurl_mime_file){name, filename, type};

#endif
