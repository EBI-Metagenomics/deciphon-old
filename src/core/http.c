/*
 * Acknowledgement: Code used bellow is mostly from
 * Apache HTTP Server soure code.
 */
#include "core/http.h"

/**
 * The size of the static status_lines array in http_protocol.c for
 * storing all of the potential response status-lines (a sparse table).
 * When adding a new code here add it to status_lines as well.
 * A future version should dynamically generate the apr_table_t at startup.
 */
#define RESPONSE_CODES 103

static const char *const status_lines[RESPONSE_CODES] = {
    "100 Continue",
    "101 Switching Protocols",
    "102 Processing",
#define LEVEL_200 3
    "200 OK",
    "201 Created",
    "202 Accepted",
    "203 Non-Authoritative Information",
    "204 No Content",
    "205 Reset Content",
    "206 Partial Content",
    "207 Multi-Status",
    "208 Already Reported",
    0, /* 209 */
    0, /* 210 */
    0, /* 211 */
    0, /* 212 */
    0, /* 213 */
    0, /* 214 */
    0, /* 215 */
    0, /* 216 */
    0, /* 217 */
    0, /* 218 */
    0, /* 219 */
    0, /* 220 */
    0, /* 221 */
    0, /* 222 */
    0, /* 223 */
    0, /* 224 */
    0, /* 225 */
    "226 IM Used",
#define LEVEL_300 30
    "300 Multiple Choices",
    "301 Moved Permanently",
    "302 Found",
    "303 See Other",
    "304 Not Modified",
    "305 Use Proxy",
    0, /* 306 */
    "307 Temporary Redirect",
    "308 Permanent Redirect",
#define LEVEL_400 39
    "400 Bad Request",
    "401 Unauthorized",
    "402 Payment Required",
    "403 Forbidden",
    "404 Not Found",
    "405 Method Not Allowed",
    "406 Not Acceptable",
    "407 Proxy Authentication Required",
    "408 Request Timeout",
    "409 Conflict",
    "410 Gone",
    "411 Length Required",
    "412 Precondition Failed",
    "413 Request Entity Too Large",
    "414 Request-URI Too Long",
    "415 Unsupported Media Type",
    "416 Requested Range Not Satisfiable",
    "417 Expectation Failed",
    "418 I'm A Teapot",
    0, /* 419 */
    0, /* 420 */
    "421 Misdirected Request",
    "422 Unprocessable Entity",
    "423 Locked",
    "424 Failed Dependency",
    "425 Too Early",
    "426 Upgrade Required",
    0, /* 427 */
    "428 Precondition Required",
    "429 Too Many Requests",
    0, /* 430 */
    "431 Request Header Fields Too Large",
    0, /* 432 */
    0, /* 433 */
    0, /* 434 */
    0, /* 435 */
    0, /* 436 */
    0, /* 437 */
    0, /* 438 */
    0, /* 439 */
    0, /* 440 */
    0, /* 441 */
    0, /* 442 */
    0, /* 443 */
    0, /* 444 */
    0, /* 445 */
    0, /* 446 */
    0, /* 447 */
    0, /* 448 */
    0, /* 449 */
    0, /* 450 */
    "451 Unavailable For Legal Reasons",
#define LEVEL_500 91
    "500 Internal Server Error",
    "501 Not Implemented",
    "502 Bad Gateway",
    "503 Service Unavailable",
    "504 Gateway Timeout",
    "505 HTTP Version Not Supported",
    "506 Variant Also Negotiates",
    "507 Insufficient Storage",
    "508 Loop Detected",
    0, /* 509 */
    "510 Not Extended",
    "511 Network Authentication Required"};

/* The index is found by its offset from the x00 code of each level.
 * Although this is fast, it will need to be replaced if some nutcase
 * decides to define a high-numbered code before the lower numbers.
 * If that sad event occurs, replace the code below with a linear search
 * from status_lines[shortcut[i]] to status_lines[shortcut[i+1]-1];
 * or use 0 to fill the gaps.
 */
static long index_of_response(long status)
{
    static long shortcut[6] = {0,         LEVEL_200, LEVEL_300,
                               LEVEL_400, LEVEL_500, RESPONSE_CODES};
    if (status < 100)
    { /* Below 100 is illegal for HTTP status */
        return -1;
    }
    if (status > 999)
    { /* Above 999 is also illegal for HTTP status */
        return -1;
    }

    for (int i = 0; i < 5; i++)
    {
        status -= 100;
        if (status < 100)
        {
            long pos = (status + shortcut[i]);
            if (pos < shortcut[i + 1] && status_lines[pos] != 0)
            {
                return pos;
            }
            else
            {
                break;
            }
        }
    }
    return -2; /* Status unknown (falls in gap) or above 600 */
}

char const *http_strcode(long code)
{
    long index = index_of_response(code);
    if (index >= 0)
    {
        return status_lines[index];
    }
    else if (index == -2)
    {
        return "unknown HTTP status code";
    }
    return status_lines[LEVEL_500];
}
