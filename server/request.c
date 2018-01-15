

#define MAX_LEN 1024

void
handle_request(void *arg)
{
    int cfd = (int) arg;
     // use getpeername() to get address of peer socket

    rbuf_t rbuf;
    struct stat stbuf;
    char buf[BUF_SIZE], method[MAX_LEN], uri[4096], proto_ver[MAX_LEN];
    char filename[4096];

    readBufInit(cfd, &rbuf);
    readLineFromBuf(&rbuf, buf, BUF_SIZE);

    /* Parse request line */
    sscanf(buf, "%s %s %s", method, uri, proto_ver);

    char tmp[MAX_LEN];
    strcpy(tmp, proto_ver);
    char *proto = strtok(tmp, "/");
    char *ver = strtok(NULL, "/");

    if (strcmp(proto, "HTTP") || strcmp(ver, "1.0") || strcmp(ver, "1.1")) {
        errMsg("handle_request(): Invalid HTTP Request protocol version");
        return;
    }

    switch (method) {
        case "GET":
            request_get()
        default :
            request_error()
            errMsg("handle_request(): Invalid HTTP Request Method");
            return;
    }

}