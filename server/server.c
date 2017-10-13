#include <signal.h>
#include <syslog.h>
#include "tlpi_hr.h"
#include "error_functions.h"
#include "inet_sockets.h"
#include "become_daemon.h"
#include "utils.h"
#include "get_num.h"


#define FD_BUF_SIZE 20  /* Obtain from command line args later */
#define SERVICE "http"
#define BACKLOG 20      /* Obtain from command line args or file later */

int
main(int argc, char *argv[])
{
    int lfd, cfd;   /* Listening and connection socket file descriptors */

        /* Create thread pool */

    lfd = inetListen(SERVICE, BACKLOG);
    if (lfd == -1) {
        errExit("inetListen");  /* Use syslog later on */
    }

    for (;;) {
        cfd = accept(lfd, NULL, NULL);
        if (cfd == -1) {
            errExit("accept");
        }

        /* Assign cfd to thread */

    }
}





