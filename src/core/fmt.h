#ifndef CORE_FMT_H
#define CORE_FMT_H

char const *fmt(char const *format, ...) __attribute__((format(printf, 1, 2)));
char *fmt_percent(char *str, int perc);

#endif
