#ifndef RUN_ONCE_H
#define RUN_ONCE_H

#define RUN_ONCE                                                               \
    do                                                                         \
    {                                                                          \
        static _Thread_local int guard##__LINE__ = 0;                          \
        if (guard##__LINE__) return;                                           \
        guard##__LINE__ = 1;                                                   \
    } while (0);

#endif
