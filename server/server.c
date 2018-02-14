
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

    /* Create a listening socket */
    socklen_t addrlen;
    lfd = inetListen(SERVICE, BACKLOG, &addrlen);
    if (lfd == -1) {
        errExit("main(): inetListen(): Failed to create a listening socket");
    }

     /* Create a thread to accept incoming signals synchronously */
    pthread_t signal_thr;
    if (pthread_create(&signal_thr, NULL, handle_signals, lfd) > 0) {
        errExit("main(): pthread_create(): Failed to create signal handling thread");
    }

    /* Detach the signal handling thread */
    if (pthread_detach(signal_thr) > 0) {
        errExit("main(): pthread_detach(): Failed to detach signal handling thread");
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


/*
This signal handler function is executed in a separate thread. It waits
for and accepts signals synchronously to initiate a graceful termination 
of this program. shutdown() is used to interrupt the blocking accept() as
it will return an error. Modifying run_forever boolean trap will then break
the loop in the main thread.

We can also implement this graceful termination by setting up a signal
handler in the main thread for the signals we want to handle. In this case,
it is important to unset the SA_RESTART flag in the sa_flags of the sigaction
structure for the handler so that accept() returns -1 upon signal reception. 
The signal handler will then modify the run_forever boolean trap.
*/
static void *
handle_signals(void *arg)
{
    //sigset_t *set = (sigset_t *) arg;
    int lfd = (int) arg;
    sigset_t set;

    if (sigaddset(&set, SIGABRT) < 0 || sigaddset(&set, SIGHUP) < 0
        || sigaddset(&set, SIGINT) < 0 || sigaddset(&set, SIGQUIT) < 0
        || sigaddset(&set, SIGTERM) < 0) {
        errExit("handle_signals(): sigaddset()");
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
            if (shutdown(lfd, SHUT_RD) < 0) {
                errExit("shutdown(): Failed to close read channel of listening socket");
            }
            break;
        case SIGABRT:
            //
            break;
        default:
            break;
    }

    return NULL;
}