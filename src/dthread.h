#ifndef DTHREAD_H
#define DTHREAD_H

#include <pthread.h>
#include <time.h>

void dthread_cond_destroy(pthread_cond_t* cond);
void dthread_cond_init(pthread_cond_t* restrict cond);
void dthread_create(pthread_t* thread, void* (*start_routine)(void*), void* arg);
void dthread_join(pthread_t thread);
void dthread_lock(pthread_mutex_t* mutex);
void dthread_mutex_destroy(pthread_mutex_t* mutex);
void dthread_mutex_init(pthread_mutex_t* restrict mutex);
void dthread_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, time_t seconds);
void dthread_unlock(pthread_mutex_t* mutex);

#endif
