#ifndef FMT_H
#define FMT_H

char const *fmt(char const *format, ...) __attribute__((format(printf, 1, 2)));
char *fmt_perc(char *str, int perc);

#endif
