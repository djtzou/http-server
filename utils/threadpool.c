#include <pthread.h>
#include "error_functions.h"
#include "threadpool.h"


typedef struct {
    void *(*job)(void *);
    void *arg;
    Job *next;
} Job;

typedef struct {
    pthread_mutex_t updateQueueMtx;
    Job *head;
    Job *tail;
    Boolean hasJobs;
    unsigned int numJobs;
    unsigned int queueSize;
} JobQueue;

typedef struct {
    int id;
    pthread_t thread;
    ThreadPool *threadPool;
} Thread;

typedef struct {
    Thread **threads;
    unsigned int totThreads;
    unsigned int aliveThreads;
    unsigned int workingThreads;
    pthread_mutex_t threadPoolMtx;
    pthread_cond_t threadPoolCnd;
    JobQueue jobQueue;
} ThreadPool;


/** Create thread pool */
ThreadPool *
threadPoolCreate(unsigned int numThreads, unsigned int jobQueueSize)
{
    if (numThreads <= 0) {
        errMsg("Must specify a positive number of threads");
        return NULL;
    }

    /* Create the thread pool */
    ThreadPool *threadPool;
    threadPool = (ThreadPool *) malloc(sizeof(*threadPool));
    if (threadPool == NULL) {
        errMsg("Failed to allocate memory for thread pool");
        return NULL;
    }

    threadPool->totThreads = numThreads;
    threadPool->aliveThreads = 0;
    threadPool->workingThreads = 0;

    /* Initialize the job queue */
    if (jobQueueInitialize(&threadPool->jobQueue, jobQueueSize) == -1) {
        errMsg("Failed to initialize the job queue");
        free(threadPool);
        return NULL;
    }

    /* Create threads in the pool */
    threadPool->threads = (Thread **) malloc(numThreads * sizeof(*threadPool->threads));
    if (threadPool->threads == NULL) {
        errMsg("Failed to allocate memory for Thread structures");
        jobQueueDestroy(&threadPool->jobQueue);
        free(threadPool);
        return NULL;
    }

    /* Initialize thread pool mutex and condition variable */
    if (pthread_mutex_init(&threadPool->threadPoolMtx, NULL) > 0) {
        errMsg("Failed to initialize thread pool mutex");
        return NULL;
    }

    if (pthread_cond_init(&threadPool->threadPoolCnd, NULL) > 0) {
        errMsg("Failed to initialize thread pool condition variable");
        return NULL;
    }

    /* Initialize threads in pool */
    int i;
    for (i = 0; i < numThreads; i++) {
        threadInitialize(threadPool, threadPool->threads[i], i);
    }





}



static int
jobQueueInitialize(JobQueue *jobQueue, unsigned int jobQueueSize)
{
    jobQueue = (JobQueue *) malloc(sizeof(*jobQueue));
    if (jobQueue == NULL) {
        errMsg("Failed to allocate memory for jobQueue");
        return -1;
    }

    if (pthread_mutex_init(&jobQueue->updateQueueMtx, NULL) > 0) {
        errMsg("Failed to initialize mutex");
        return -1;
    }

    jobQueue->head = NULL;
    jobQueue->tail = NULL:
    jobQueue->hasJobs = FALSE;
    jobQueue->numJobs = 0;
    jobQueue->queueSize = jobQueueSize;

    return 0;
}


static void
jobQueueDestroy(JobQueue *jobQueue)
{
    /* Free all jobs in queue */
    Job *head = jobQueue->head;
    while (head != NULL) {
        Job *tmp = head->next;
        free(head);
        head = tmp;
    }

    free(jobQueue);
    return 0
}




static int
threadInitialize(ThreadPool *threadPool, Thread *thread, int id)
{
    thread = (Thread *) malloc(sizeof(*thread));
    if (thread == NULL) {
        errMsg("Failed to allocate memory for Thread structure");
        return -1;
    }

    thread->id = id;
    thread->threadPool = threadPool;

    if (pthread_create(&thread->thread, NULL, &threadStart, (void *) thread) > 0) {
        errMsg("Failed to create a new thread");
        return -1;
    }

    return 0;
}


static void *
threadStart(void *arg)
{
    Thread *thread = (Thread *) arg;
    ThreadPool *threadPool = thread->threadPool;

}

static void
threadDestroy(Thread *thread)
{

}



threadpool_add_job();
threadpool_wait();
threadpool_num_of_workers();
threadpool_destroy();
    threadpool_free();