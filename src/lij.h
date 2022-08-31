// Amalgamation of the following files:
//    lij_pack.h
/* --- lij_pack section ----------------------------------- */

#ifndef LIJ_PACK_H
#define LIJ_PACK_H

#include <stdbool.h>

unsigned lij_pack_bool(char buf[], bool val);
unsigned lij_pack_int(char buf[], long val);
unsigned lij_pack_null(char buf[]);
unsigned lij_pack_str(char buf[], char const val[]);

unsigned lij_pack_object_open(char buf[]);
unsigned lij_pack_object_close(char buf[]);

unsigned lij_pack_array_open(char buf[]);
unsigned lij_pack_array_close(char buf[]);

unsigned lij_pack_comma(char buf[]);
unsigned lij_pack_colon(char buf[]);

#endif
