
#include "../utils/tlpi_hdr.h"
#include "../utils/inet_sockets.h"
#include "../threadpool/threadpool.h"
#include "request.h"
#include <signal.h>
#include <pthread.h>


//#define FD_BUF_SIZE 20  /* Obtain from command line args later */
#define SERVICE "http"
#define BACKLOG 20      /* Obtain from command line args or file later */
#define NUM_THREADS 4
#define MAX_NUM_JOBS 100


static volatile sig_atomic_t run_forever = 1;
static void *handle_signals();


int
main(int argc, char *argv[])
{
    int lfd, cfd;   /* Listening and connection socket file descriptors */

    /* Create signal mask to block delivery of signals to threads in thread pool */
    sigset_t set;
    if (sigfillset(&set) < 0) {
        errExit("main(): sigfillset()");
    }

    /* Block all signals before creating threads */
    if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
        errExit("main(): sigprocmask(): Failed to create signal mask");
    }

    /* Create thread pool */
    threadpool thpool = thpool_init(NUM_THREADS, MAX_NUM_JOBS);
    if (thpool == NULL) {
        errExit("main(): thpool_init(): Failed to create thread pool");
    }

    /* Create a thread to accept incoming signals synchronously */
    pthread_t signal_thr;
    if (pthread_create(&signal_thr, NULL, handle_signals, NULL /*(void *) &set*/) > 0) {
        errExit("main(): pthread_create(): Failed to create signal handling thread");
    }

    /* Detach the signal handling thread */
    if (pthread_detach(signal_thr) > 0) {
        errExit("main(): pthread_detach(): Failed to detach signal handling thread");
    }

    /* Create a listening socket */
    socklen_t addrlen;
    lfd = inetListen(SERVICE, BACKLOG, &addrlen);
    if (lfd == -1) {
        errExit("main(): inetListen(): Failed to create a listening socket");
    }

    for (;run_forever;) {

        /* Accept incoming connections */
        cfd = accept(lfd, NULL, NULL);
        if (cfd == -1) {
            errMsg("main(): Failed to accept connection");
            continue;
        }

        /* Assign connection to a thread */
        if (thpool_add_work(thpool, request_handle, cfd) == -1) {
            errMsg("main(): Failed to add work to thread pool");
            // Should we wait for one job to complete? add this new method to threadpool API?
        }
    }

    thpool_destroy(thpool);
    close(lfd);

    exit(EXIT_SUCCESS);
}

static void *
handle_signals(/*void *arg*/)
{
    //sigset_t *set = (sigset_t *) arg;
    sigset_t set, prev_set;

    if (sigaddset(&set, SIGABRT) < 0 || sigaddset(&set, SIGHUP) < 0
        || sigaddset(&set, SIGINT) < 0 || sigaddset(&set, SIGQUIT) < 0
        || sigaddset(&set, SIGTERM) < 0) {
        errExit("handle_signals(): sigaddset()");
    }

    if (pthread_sigmask(SIG_UNBLOCK, &set, &prev_set) > 0) {
        errExit("handle_signals(): pthread_sigmask(): Failed to unblock signals");
    }

    int sig;
    if (sigwait(&set, &sig) > 0) {
        errExit("handle_signals(): sigwait()");
    }

    switch (sig) {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
        case SIGHUP:
            run_forever = 0;
            break;
        case SIGABRT:
            //
            break;
        default:
            break;
    }

    return NULL;
}