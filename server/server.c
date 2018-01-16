
#include "tlpi_hr.h"
#include "inet_sockets.h"
#include "../threadpool/threadpool.h"
#include "request.h"


#define FD_BUF_SIZE 20  /* Obtain from command line args later */
#define SERVICE "http"
#define BACKLOG 20      /* Obtain from command line args or file later */
#define NUM_THREADS 4
#define MAX_NUM_JOBS 100


int
main(int argc, char *argv[])
{
    int lfd, cfd;   /* Listening and connection socket file descriptors */

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

    for (;;) {

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
}





