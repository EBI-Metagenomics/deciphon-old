#ifndef DIE_H
#define DIE_H

_Noreturn void __die(char const *file, int line, char const *func);

#define die()                                                                  \
    do                                                                         \
    {                                                                          \
        __die(__FILE__, __LINE__, __func__);                                   \
    } while (0)

#endif
