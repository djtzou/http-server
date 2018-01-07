/************************************************\
 * Based on Pithikos/C-Thread-Pool implemenation *
 * by Johan Hanssen Seferidis                    *
 * Modified by Deh-Jun Tzou                      *
\************************************************/

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include "tlpi_hdr.h"
#include "threadpool.h"


/* ========================== GLOBALS ============================ */


static volatile int threads_keepalive;  // Must declare as volatile to prevent compiler optimization


/* ========================== STRUCTURES ============================ */


/* Binary Semaphore */
typedef struct bsem {
    pthread_mutex_t mtx;
    pthread_cond_t cnd;
    int val;
} bsem;

/* Job */
typedef struct job {
    void (*function)(void *);
    void *arg;
    struct job *next;
} job;

/* Job Queue */
typedef struct jobqueue {
    pthread_mutex_t jobqueue_mtx;
    job *head;
    job *tail;
    bsem *has_jobs;
    unsigned int num_jobs;
    unsigned int max_size;
} jobqueue;

/* Thread */
typedef struct thread {
    int id;
    pthread_t pthread;
    struct thpool *thpool;
} thread;

/* Thread Pool */
typedef struct thpool {
    thread **threads;
    unsigned int num_alive_threads;
    unsigned int num_working_threads;
    pthread_mutex_t thpool_mtx;
    pthread_cond_t thpool_cnd;
    jobqueue jobqueue;
} thpool;


/* ========================== PROTOTYPES ============================ */


static int jobqueue_init(jobqueue *jobqueue_p, unsigned int jobqueue_size);
static void jobqueue_destroy(jobqueue *jobqueue_p);
static int jobqueue_clear(jobqueue *jobqueue_p);
static job *jobqueue_poll(jobqueue *jobqueue_p);
static int jobqueue_add(jobqueue *jobqueue_p, job *job_p);

static int thread_init(thpool *thpool_p, thread **thread_p, int id);
static void *thread_start(void *arg);
static void thread_destroy(thread *thread_p);

static int bsem_init(bsem *bsem_p, int val);
static int bsem_reset(bsem *bsem_p);
static void bsem_post(bsem *bsem_p);
static void bsem_post_all(bsem *bsem_p);
static void bsem_wait(bsem *bsem_p);


/* ========================== THREAD POOL ========================== */


/* Create thread pool */
thpool *
thpool_init(unsigned int num_threads, unsigned jobqueue_size)
{
    if (num_threads <= 0) {
        errMsg("Must specify a positive number of threads");
        return NULL;
    }

    /* Create the thread pool */
    thpool *thpool_p;
    thpool_p = (thpool *) malloc(sizeof(*thpool_p));
    if (thpool_p == NULL) {
        errMsg("Failed to allocate memory for thread pool");
        return NULL;
    }

    thpool_p->num_alive_threads = 0;
    thpool_p->num_working_threads = 0;

    /* Initialize the job queue */
    if (jobqueue_init(&thpool_p->jobqueue, jobqueue_size) == -1) {
        errMsg("Failed to initialize the job queue");
        free(thpool_p);
        return NULL;
    }

    /* Create threads in the pool */
    thpool_p->threads = (thread **) malloc(num_threads * sizeof(*thpool_p->threads));
    if (thpool_p->threads == NULL) {
        errMsg("Failed to allocate memory for thread structures");
        jobqueue_destroy(&thpool_p->jobqueue);
        free(thpool_p);
        return NULL;
    }

    /* Initialize thread pool mutex and condition variable */
    if (pthread_mutex_init(&thpool_p->thpool_mtx, NULL) > 0) {
        errMsg("Failed to initialize thread pool mutex");
        return NULL;
    }

    if (pthread_cond_init(&thpool_p->thpool_cnd, NULL) > 0) {
        errMsg("Failed to initialize thread pool condition variable");
        return NULL;
    }

    /* Initialize threads in pool */
    int i;
    for (i = 0; i < num_threads; i++) {
        thread_init(thpool_p, &thpool_p->threads[i], i);
    }

    /* Wait for all threads to be initialized */
    while (thpool_p->num_alive_threads != num_threads);

    return thpool_p;
}

int
thpool_add_work(thpool *thpool_p, void (*function)(void*), void *arg)
{
    job *new_job;
    new_job = (job *) malloc(sizeof(*new_job));
    if (new_job == NULL) {
        errMsg("thpool_add_work(): Failed to allocate memory for new job");
        return -1;
    }

    new_job->function = function;
    new_job->arg = arg;

    if (jobqueue_add(&thpool_p->jobqueue, new_job) < 0) {
        errMsg("thpool_add_work(): Failed to add new job to job queue");
        free(new_job);
        return -1;
    }

    return 0;
}

void
thpool_destroy(thpool *thpool_p)
{
    if (thpool_p == NULL)
        return;

    /* Get total number of alive threads */
    unsigned int total_alive_threads = thpool_p->num_alive_threads;

    /* End infinite loop for each thread */
    threads_keepalive = 0;

    /* Wait a second for threads to terminate */
    bsem_post_all(thpool_p->jobqueue.has_jobs);
    sleep(1);

    /* Terminate all remaining threads */
    while (thpool_p->num_alive_threads) {
        bsem_post_all(thpool_p->jobqueue.has_jobs);
        sleep(1);
    }

    /* Destroy job queue */
    jobqueue_destroy(&thpool_p->jobqueue);

    /* Destroy thread structures */
    int i;
    for (i = 0; i < total_alive_threads; i++) {
        thread_destroy(thpool_p->threads[i]);
    }

    free(thpool_p->threads);
    free(thpool_p);
}

void
thpool_wait(thpool *thpool_p)
{
    pthread_mutex_lock(&thpool_p->thpool_mtx);
    while (thpool_p->jobqueue.num_jobs || thpool_p->num_working_threads) {
        pthread_cond_wait(&thpool_p->thpool_cnd, &thpool_p->thpool_mtx);
    }
    pthread_mutex_unlock(&thpool_p->thpool_mtx);
}

int
thpool_num_working_threads(thpool *thpool_p)
{
    return thpool_p->num_working_threads;
}


/* ========================== JOB QUEUE ========================== */


static int
jobqueue_init(jobqueue *jobqueue_p, unsigned int jobqueue_size)
{
    if (pthread_mutex_init(&jobqueue_p->jobqueue_mtx, NULL) > 0) {
        errMsg("jobqueue_init(): Failed to initialize job queue mutex");
        return -1;
    }

    jobqueue_p->has_jobs = (bsem *) malloc(sizeof(*jobqueue_p->has_jobs));
    if (jobqueue_p->has_jobs == NULL) {
        errMsg("jobqueue_init(): Failed to allocate memory for binary semaphore");
        return -1;
    }

    if (bsem_init(jobqueue_p->has_jobs, 0) == -1) {
        errMsg("jobqueue_init(): Failed to allocate memory for binary semaphore");
        free(jobqueue_p->has_jobs);
        return -1;
    }

    jobqueue_p->head = NULL;
    jobqueue_p->tail = NULL;
    jobqueue_p->num_jobs = 0;
    jobqueue_p->max_size = jobqueue_size;

    return 0;
}

static void
jobqueue_destroy(jobqueue *jobqueue_p)
{
    jobqueue_clear(jobqueue_p);
    free(jobqueue_p->has_jobs);
}

static int
jobqueue_clear(jobqueue *jobqueue_p)
{
    job *job_p;
    while ((job_p = jobqueue_poll(jobqueue_p)) != NULL) {
        free(job_p);
    }

    if (bsem_reset(jobqueue_p->has_jobs) == -1) {
        errMsg("jobqueue_clear(): Failed to reset binary semaphore");
        return -1;
    }

    jobqueue_p->num_jobs = 0;
    jobqueue_p->head = NULL;
    jobqueue_p->tail = NULL;

    return 0;
}

static job *
jobqueue_poll(jobqueue *jobqueue_p)
{
    pthread_mutex_lock(&jobqueue_p->jobqueue_mtx);
    job *head = jobqueue_p->head;
    if (jobqueue_p->num_jobs == 1) {
        jobqueue_p->head = NULL;
        jobqueue_p->tail = NULL;
        jobqueue_p->num_jobs = 0;
    }
    else if (jobqueue_p->num_jobs > 1) {
        jobqueue_p->head = head->next;
        jobqueue_p->num_jobs--;
        bsem_post(jobqueue_p->has_jobs);
    }
    pthread_mutex_unlock(&jobqueue_p->jobqueue_mtx);

    return head;
}

static int
jobqueue_add(jobqueue *jobqueue_p, job *job_p)
{
    int ret_val = -1;
    pthread_mutex_lock(&jobqueue_p->jobqueue_mtx);
    job_p->next = NULL;
    if (jobqueue_p->num_jobs < jobqueue_p->max_size)
    {
        if (jobqueue_p->head == NULL)
        {
            jobqueue_p->head = job_p;
            jobqueue_p->tail = job_p;
        }
        else
        {
            jobqueue_p->tail->next = job_p;
            jobqueue_p->tail = job_p;
        }

        jobqueue_p->num_jobs++;
        bsem_post(jobqueue_p->has_jobs);
        ret_val = 0;
    }
    pthread_mutex_unlock(&jobqueue_p->jobqueue_mtx);

    return ret_val;
}


/* ========================== THREAD ========================== */


static int
thread_init(thpool *thpool_p, thread **thread_p, int id)
{   // Must use double pointer (thread **thread) to fill entry in thpool->threads
    
    *thread_p = (thread *) malloc(sizeof(**thread_p));
    if (*thread_p == NULL) {
        errMsg("thread_init(): Failed to allocate memory for thread structure");
        return -1;
    }

    (*thread_p)->id = id;
    (*thread_p)->thpool = thpool_p;

    if (pthread_create(&(*thread_p)->pthread, NULL, thread_start, *thread_p) > 0) {
        errMsg("thread_init(): Failed to create new thread");
        free(*thread_p);
        return -1;
    }

    if (pthread_detach((*thread_p)->pthread) > 0) {
        errMsg("thread_init(): Failed to detach thread");
        free(*thread_p);
        return -1;
    }

    return 0;
}

static void *
thread_start(void *arg)
{
    thread *thread_p = (thread *) arg;

    /* Set thread name? */


    thpool *thpool_p = thread_p->thpool;

    /* Mark thread as alive */
    pthread_mutex_lock(&thpool_p->thpool_mtx);
    thpool_p->num_alive_threads++;
    pthread_mutex_unlock(&thpool_p->thpool_mtx);

    while (threads_keepalive) {

        bsem_wait(thpool_p->jobqueue.has_jobs);

        if (threads_keepalive) {    // Check invariant again as state may have changed

            pthread_mutex_lock(&thpool_p->thpool_mtx);
            thpool_p->num_working_threads++;
            pthread_mutex_unlock(&thpool_p->thpool_mtx);

            job *job_p = jobqueue_poll(&thpool_p->jobqueue);
            void (*function)(void *);
            void *arg;
            if (job_p != NULL) {
                function = job_p->function;
                arg = job_p->arg;
                function(arg);
                free(job_p);
            }

            pthread_mutex_lock(&thpool_p->thpool_mtx);
            thpool_p->num_working_threads--;
            if (thpool_p->num_working_threads == 0) {
                pthread_cond_signal(&thpool_p->thpool_cnd);    // Signal thpool_wait()
            }
            pthread_mutex_unlock(&thpool_p->thpool_mtx);
        }
    }

    pthread_mutex_lock(&thpool_p->thpool_mtx);
    thpool_p->num_alive_threads--;
    pthread_mutex_unlock(&thpool_p->thpool_mtx);

    return NULL;
}

static void
thread_destroy(thread *thread_p)
{
    free(thread_p);
}


/* ========================== SYNCHRONIZATION ========================== */


static int
bsem_init(bsem *bsem_p, int val)
{
    if (val < 0 || val > 1) {
        errMsg("bsem_p_init(): Binary semaphore value must be 0 or 1");
        return -1;
    }

    if (pthread_mutex_init(&bsem_p->mtx, NULL) > 0) {
        errMsg("bsem_p_init(): Failed to initialize binary semaphore mutex");
        return -1;
    }

    if (pthread_cond_init(&bsem_p->cnd, NULL) > 0) {
        errMsg("bsem_p_init(): Failed to initialize binary semaphore condition variable");
        return -1;
    }

    bsem_p->val = val;

    return 0;
}

static int
bsem_reset(bsem *bsem_p)
{
    return bsem_init(bsem_p, 0);
}

static void
bsem_post(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mtx);
    bsem_p->val = 1;
    pthread_mutex_unlock(&bsem_p->mtx);
    pthread_cond_signal(&bsem_p->cnd);
}

static void
bsem_post_all(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mtx);
    bsem_p->val = 1;
    pthread_mutex_unlock(&bsem_p->mtx);
    pthread_cond_broadcast(&bsem_p->cnd);
}

static void
bsem_wait(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mtx);
    while (bsem_p->val != 1) {
        pthread_cond_wait(&bsem_p->cnd, &bsem_p->mtx);
    }
    bsem_p->val = 0;
    pthread_mutex_unlock(&bsem_p->mtx);
}