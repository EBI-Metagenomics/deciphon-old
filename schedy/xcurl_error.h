#ifndef SCHEDY_XCURL_ERROR_H
#define SCHEDY_XCURL_ERROR_H

#include "logy.h"
#include <curl/curl.h>

/* clang-format off */
#define xcurl_error(code)                                                      \
    code == CURLE_OK                                                           \
        ? efail("No error")                                                    \
    : code == CURLE_UNSUPPORTED_PROTOCOL                                       \
        ? efail("Unsupported protocol")                                        \
    : code == CURLE_FAILED_INIT                                                \
        ? efail("Failed initialization")                                       \
    : code == CURLE_URL_MALFORMAT                                              \
        ? efail("URL using bad/illegal format or missing URL")                 \
    : code == CURLE_NOT_BUILT_IN                                               \
        ? efail("A requested feature, protocol or option was not "             \
                "found built-in in this libcurl due to a build-time decision.")\
    : code == CURLE_COULDNT_RESOLVE_PROXY                                      \
        ? efail("Couldn't resolve proxy name")                                 \
    : code == CURLE_COULDNT_RESOLVE_HOST                                       \
        ? efail("Couldn't resolve host name")                                  \
    : code == CURLE_COULDNT_CONNECT                                            \
        ? efail("Couldn't connect to server")                                  \
    : code == CURLE_WEIRD_SERVER_REPLY                                         \
        ? efail("Weird server reply")                                          \
    : code == CURLE_REMOTE_ACCESS_DENIED                                       \
        ? efail("Access denied to remote resource")                            \
    : code == CURLE_FTP_ACCEPT_FAILED                                          \
        ? efail("FTP: The server failed to connect to data port")              \
    : code == CURLE_FTP_ACCEPT_TIMEOUT                                         \
        ? efail("FTP: Accepting server connect has timed out")                 \
    : code == CURLE_FTP_PRET_FAILED                                            \
        ? efail("FTP: The server did not accept the PRET command.")            \
    : code == CURLE_FTP_WEIRD_PASS_REPLY                                       \
        ? efail("FTP: unknown PASS reply")                                     \
    : code == CURLE_FTP_WEIRD_PASV_REPLY                                       \
        ? efail("FTP: unknown PASV reply")                                     \
    : code == CURLE_FTP_WEIRD_227_FORMAT                                       \
        ? efail("FTP: unknown 227 response format")                            \
    : code == CURLE_FTP_CANT_GET_HOST                                          \
        ? efail("FTP: can't figure out the host in the PASV "                  \
                "response")                                                    \
    : code == CURLE_HTTP2                                                      \
        ? efail("Error in the HTTP2 framing layer")                            \
    : code == CURLE_FTP_COULDNT_SET_TYPE                                       \
        ? efail("FTP: couldn't set file type")                                 \
    : code == CURLE_PARTIAL_FILE                                               \
        ? efail("Transferred a partial file")                                  \
    : code == CURLE_FTP_COULDNT_RETR_FILE                                      \
        ? efail("FTP: couldn't retrieve (RETR failed) the "                    \
                "specified file")                                              \
    : code == CURLE_QUOTE_ERROR                                                \
        ? efail("Quote command returned error")                                \
    : code == CURLE_HTTP_RETURNED_ERROR                                        \
        ? efail("HTTP response code said error")                               \
    : code == CURLE_WRITE_ERROR                                                \
        ? efail("Failed writing received data to disk/application")            \
    : code == CURLE_UPLOAD_FAILED                                              \
        ? efail("Upload failed (at start/before it took off)")                 \
    : code == CURLE_READ_ERROR                                                 \
        ? efail("Failed to open/read local data from "                         \
                "file/application")                                            \
    : code == CURLE_OUT_OF_MEMORY                                              \
        ? efail("Out of memory")                                               \
    : code == CURLE_OPERATION_TIMEDOUT                                         \
        ? efail("Timeout was reached")                                         \
    : code == CURLE_FTP_PORT_FAILED                                            \
        ? efail("FTP: command PORT failed")                                    \
    : code == CURLE_FTP_COULDNT_USE_REST                                       \
        ? efail("FTP: command REST failed")                                    \
    : code == CURLE_RANGE_ERROR                                                \
        ? efail("Requested range was not delivered by the server")             \
    : code == CURLE_HTTP_POST_ERROR                                            \
        ? efail("Internal problem setting up the POST")                        \
    : code == CURLE_SSL_CONNECT_ERROR                                          \
        ? efail("SSL connect error")                                           \
    : code == CURLE_BAD_DOWNLOAD_RESUME                                        \
        ? efail("Couldn't resume download")                                    \
    : code == CURLE_FILE_COULDNT_READ_FILE                                     \
        ? efail("Couldn't read a file:// file")                                \
    : code == CURLE_LDAP_CANNOT_BIND                                           \
        ? efail("LDAP: cannot bind")                                           \
    : code == CURLE_LDAP_SEARCH_FAILED                                         \
        ? efail("LDAP: search failed")                                         \
    : code == CURLE_FUNCTION_NOT_FOUND                                         \
        ? efail("A required function in the library was not found")            \
    : code == CURLE_ABORTED_BY_CALLBACK                                        \
        ? efail("Operation was aborted by an application callback")            \
    : code == CURLE_BAD_FUNCTION_ARGUMENT                                      \
        ? efail("A libcurl function was given a bad argument")                 \
    : code == CURLE_INTERFACE_FAILED                                           \
        ? efail("Failed binding local connection end")                         \
    : code == CURLE_TOO_MANY_REDIRECTS                                         \
        ? efail("Number of redirects hit maximum amount")                      \
    : code == CURLE_UNKNOWN_OPTION                                             \
        ? efail("An unknown option was passed in to libcurl")                  \
    : code == CURLE_GOT_NOTHING                                                \
        ? efail("Server returned nothing (no headers, no data)")               \
    : code == CURLE_SSL_ENGINE_NOTFOUND                                        \
        ? efail("SSL crypto engine not found")                                 \
    : code == CURLE_SSL_ENGINE_SETFAILED                                       \
        ? efail("Can not set SSL crypto engine as default")                    \
    : code == CURLE_SSL_ENGINE_INITFAILED                                      \
        ? efail("Failed to initialise SSL crypto engine")                      \
    : code == CURLE_SEND_ERROR                                                 \
        ? efail("Failed sending data to the peer")                             \
    : code == CURLE_RECV_ERROR                                                 \
        ? efail("Failure when receiving data from the peer")                   \
    : code == CURLE_SSL_CERTPROBLEM                                            \
        ? efail("Problem with the local SSL certificate")                      \
    : code == CURLE_SSL_CIPHER                                                 \
        ? efail("Couldn't use specified SSL cipher")                           \
    : code == CURLE_PEER_FAILED_VERIFICATION                                   \
        ? efail("SSL peer certificate or SSH remote key was not OK")           \
    : code == CURLE_SSL_CACERT_BADFILE                                         \
        ? efail("Problem with the SSL CA cert (path? access "                  \
                "rights?)")                                                    \
    : code == CURLE_BAD_CONTENT_ENCODING                                       \
        ? efail("Unrecognized or bad HTTP Content or "                         \
                "Transfer-Encoding")                                           \
    : code == CURLE_FILESIZE_EXCEEDED                                          \
        ? efail("Maximum file size exceeded")                                  \
    : code == CURLE_USE_SSL_FAILED                                             \
        ? efail("Requested SSL level failed")                                  \
    : code == CURLE_SSL_SHUTDOWN_FAILED                                        \
        ? efail("Failed to shut down the SSL connection")                      \
    : code == CURLE_SSL_CRL_BADFILE                                            \
        ? efail("Failed to load CRL file (path? access rights?,"               \
                " format?)")                                                   \
    : code == CURLE_SSL_ISSUER_ERROR                                           \
        ? efail("Issuer check against peer certificate failed")                \
    : code == CURLE_SEND_FAIL_REWIND                                           \
        ? efail("Send failed since rewinding of the data"                      \
                   " stream failed")                                           \
    : code == CURLE_LOGIN_DENIED                                               \
        ? efail("Login denied")                                                \
    : code == CURLE_TFTP_NOTFOUND                                              \
        ? efail("TFTP: File Not Found")                                        \
    : code == CURLE_TFTP_PERM                                                  \
        ? efail("TFTP: Access Violation")                                      \
    : code == CURLE_REMOTE_DISK_FULL                                           \
        ? efail("Disk full or allocation exceeded")                            \
    : code == CURLE_TFTP_ILLEGAL                                               \
        ? efail("TFTP: Illegal operation")                                     \
    : code == CURLE_TFTP_UNKNOWNID                                             \
        ? efail("TFTP: Unknown transfer ID")                                   \
    : code == CURLE_REMOTE_FILE_EXISTS                                         \
        ? efail("Remote file already exists")                                  \
    : code == CURLE_TFTP_NOSUCHUSER                                            \
        ? efail("TFTP: No such user")                                          \
    : code == CURLE_CONV_FAILED                                                \
        ? efail("Conversion failed")                                           \
    : code == CURLE_REMOTE_FILE_NOT_FOUND                                      \
        ? efail("Remote file not found")                                       \
    : code == CURLE_SSH                                                        \
        ? efail("Error in the SSH layer")                                      \
    : code == CURLE_AGAIN                                                      \
        ? efail("Socket not ready for send/recv")                              \
    : code == CURLE_RTSP_CSEQ_ERROR                                            \
        ? efail("RTSP CSeq mismatch or invalid CSeq")                          \
    : code == CURLE_RTSP_SESSION_ERROR                                         \
        ? efail("RTSP session error")                                          \
    : code == CURLE_FTP_BAD_FILE_LIST                                          \
        ? efail("Unable to parse FTP file list")                               \
    : code == CURLE_CHUNK_FAILED                                               \
        ? efail("Chunk callback failed")                                       \
    : code == CURLE_NO_CONNECTION_AVAILABLE                                    \
        ? efail("The max connection limit is reached")                         \
    : code == CURLE_SSL_PINNEDPUBKEYNOTMATCH                                   \
        ? efail("SSL public key does not match pinned public key")             \
    : code == CURLE_SSL_INVALIDCERTSTATUS                                      \
        ? efail("SSL server certificate status verification FAILED")           \
    : code == CURLE_HTTP2_STREAM                                               \
        ? efail("Stream error in the HTTP/2 framing layer")                    \
    : code == CURLE_RECURSIVE_API_CALL                                         \
        ? efail("API function called from within callback")                    \
    : code == CURLE_AUTH_ERROR                                                 \
        ? efail("An authentication function returned an error")                \
    : code == CURLE_HTTP3                                                      \
        ? efail("HTTP/3 error")                                                \
    : efail("Unknown error")
/* clang-format on */

#endif
