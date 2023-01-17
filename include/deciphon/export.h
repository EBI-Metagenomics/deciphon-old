#ifndef DECIPHON_API_H
#define DECIPHON_API_H

/* clang-format off */
#ifdef DECIPHON_STATIC_DEFINE
#  define DCP_API
#else
#  ifdef deciphon_EXPORTS /* We are building this library */
#    ifdef _WIN32
#      define DCP_API __declspec(dllexport)
#    else
#      define DCP_API __attribute__((visibility("default")))
#    endif
#  else /* We are using this library */
#    ifdef _WIN32
#      define DCP_API __declspec(dllimport)
#    else
#      define DCP_API __attribute__((visibility("default")))
#    endif
#  endif
#endif
/* clang-format on */

#endif
