/************************************************\
 * Based on Pithikos/C-Thread-Pool implemenation *
 * by Johan Hanssen Seferidis                    *
\************************************************/
#include <pthread.h>
#include "error_functions.h"
#include "threadpool.h"


/* ========================== STRUCTURES ============================ */


/* Job */
typedef struct {
    void (*function)(void *);
    void *arg;
    struct job *next;
} job;

/* Job Queue */
typedef struct {
    pthread_mutex_t jobqueue_mtx;
    job *head;
    job *tail;
    Boolean has_jobs;
    unsigned int num_jobs;
    unsigned int max_size;
} jobqueue;

/* Thread */
typedef struct {
    int id;
    pthread_t pthread;
    struct thpool *thpool;
} thread;

/* Thread Pool */
typedef struct {
    thread **threads;
    unsigned int num_alive_threads;
    unsigned int num_working_threads;
    pthread_mutex_t thpool_mtx;
    pthread_cond_t thpool_cnd;
    jobqueue jobqueue;
} thpool;


/* ========================== THREAD POOL ========================== */


/* Create thread pool */
thpool *
thpool_create(unsigned int num_threads, unsigned jobqueue_size)
{
    if (num_threads <= 0) {
        errMsg("Must specify a positive number of threads");
        return NULL;
    }

    /* Create the thread pool */
    thpool *thpool;
    thpool = (thpool *) malloc(sizeof(*thpool));
    if (thpool == NULL) {
        errMsg("Failed to allocate memory for thread pool");
        return NULL;
    }

    thpool->num_alive_threads = 0;
    thpool->num_working_threads = 0;

    /* Initialize the job queue */
    if (jobqueue_init(&thpool_jobqueue, jobqueue_size) == -1) {
        errMsg("Failed to initialize the job queue");
        free(thpool);
        return NULL;
    }

    /* Create threads in the pool */
    thpool->threads = (thread **) malloc(num_threads * sizeof(*thpool->threads));
    if (thpool->threads == NULL) {
        errMsg("Failed to allocate memory for thread structures");
        jobqueue_destroy(&thpool->jobqueue);
        free(thpool);
        return NULL;
    }

    /* Initialize thread pool mutex and condition variable */
    if (pthread_mutex_init(&thpool->thpool_mtx, NULL) > 0) {
        errMsg("Failed to initialize thread pool mutex");
        return NULL;
    }

    if (pthread_cond_init(&thpool->thpool_cnd, NULL) > 0) {
        errMsg("Failed to initialize thread pool condition variable");
        return NULL;
    }

    /* Initialize threads in pool */
    int i;
    for (i = 0; i < num_threads; i++) {
        thread_init(thpool, thpool->threads[i], i);
    }

    /* Wait for all threads to be initialized */
    while (thpool->num_alive_threads != num_threads);

    return thpool;
}


struct thpool_* thpool_init(int num_threads)
int thpool_add_work(thpool_* thpool_p, void (*function_p)(void*), void* arg_p)
void thpool_destroy(thpool_* thpool_p)
int thpool_num_threads_working(thpool_* thpool_p)


/* ========================== JOB QUEUE ========================== */


static int
jobqueue_init(jobqueue *jobqueue, unsigned int jobqueue_size)
{
    if (pthread_mutex_init(&(jobqueue->jobqueue_mtx), NULL) > 0) {
        errMsg("Failed to initialize job queue mutex");
        return -1;
    }

    jobqueue->head = NULL;
    jobqueue->tail = NULL;
    jobqueue->has_jobs = FALSE;
    jobqueue->num_jobs = 0;
    jobqueue->max_size = jobqueue_size;

    return 0;
}

static void
jobqueue_destroy(jobqueue *jobqueue)
{
    jobqueue_clear(jobqueue);
    free(jobqueue);
}

static void
jobqueue_clear(jobqueue *jobqueue)
{
    while ((job = jobqueue_poll(jobqueue)) != NULL) {
        free(job);
    }

    jobqueue->has_jobs = FALSE;
    jobqueue->num_jobs = 0;
    jobqueue->head = NULL;
    jobqueue->tail = NULL;
}

static job *
jobqueue_poll(jobqueue *jobqueue)
{
    pthread_mutex_lock(&jobqueue->jobqueue_mtx);
    job *head = jobqueue->head;
    if (jobqueue->num_jobs == 1) {
        jobqueue->head = NULL;
        jobqueue->tail = NULL;
        jobqueue->num_jobs = 0;
    }
    else if (jobqueue->num_jobs > 1) {
        jobqueue->head = head->next;
        jobqueue->num_jobs--;
    }
    pthread_mutex_unlock(&jobqueue->jobqueue_mtx);

    return head;
}

static int
jobqueue_add(jobqueue *jobqueue, job *job)
{
    int ret_val = -1;
    pthread_mutex_lock(&jobqueue->jobqueue_mtx);
    job->next = NULL;
    if (jobqueue->num_jobs < jobqueue->max_size)
    {
        if (jobqueue->head == NULL)
        {
            jobqueue->head = job;
            jobqueue->tail = job;
        }
        else
        {
            jobqueue->tail->next = job;
            jobqueue->tail = job;
        }

        jobqueue->num_jobs++;
        retval = 0;
    }
    pthread_mutex_unlock(&jobqueue->jobqueue_mtx);
    return ret_val;
}


/* ========================== THREAD ========================== */



static int  thread_init(thpool_* thpool_p, struct thread** thread_p, int id);
static void* thread_do(struct thread* thread_p);
static void  thread_destroy(struct thread* thread_p);