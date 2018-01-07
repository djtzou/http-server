/************************************************\
 * Header file for threadpool.c                 *
\************************************************/

#ifndef THREADPOOL_H
#define THREADPOOL_H

thpool *thpool_init(unsigned int num_threads, unsigned jobqueue_size);
int thpool_add_work(thpool *thpool, void (*function)(void*), void *arg);
void thpool_destroy(thpool* thpool);
void thpool_wait(thpool *thpool);
int thpool_num_working_threads(thpool *thpool);

#endif