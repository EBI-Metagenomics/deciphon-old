#ifndef FILE_H
#define FILE_H

#include <stdio.h>

int file_copy_content(FILE* dst, FILE* src);
int file_read_string(FILE* stream, char* str, size_t max_size);

#endif
