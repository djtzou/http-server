/************************************************\
 * Header file for threadpool.c                 *
\************************************************/

#ifndef THREADPOOL_H
#define THREADPOOL_H


typedef struct thpool * threadpool;

threadpool thpool_init(unsigned int num_threads, unsigned jobqueue_size);
int thpool_add_work(threadpool thpool_p, void (*function)(void*), void *arg);
void thpool_destroy(threadpool thpool_p);
void thpool_wait(threadpool thpool_p);
int thpool_num_working_threads(threadpool thpool_p);


#endif
