#include "core/xcurl_mime.h"

curl_mime *xcurl_mime_new_file(CURL *curl, struct xcurl_mime_file const *mime,
                               char const *filepath)
{
    curl_mime *form = curl_mime_init(curl);

    curl_mimepart *field = curl_mime_addpart(form);
    curl_mime_name(field, "name");
    curl_mime_data(field, mime->name, CURL_ZERO_TERMINATED);

    field = curl_mime_addpart(form);
    curl_mime_name(field, mime->name);
    curl_mime_type(field, mime->type);
    curl_mime_filedata(field, filepath);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "filename");
    curl_mime_data(field, mime->filename, CURL_ZERO_TERMINATED);

    return form;
}

void xcurl_mime_del(curl_mime *form) { curl_mime_free(form); }
