
#include "../utils/utils.h"

#define MAX_LEN 1024

typedef struct hdr_t {
    char *field_name;
    char *field_value;
    struct hdr_t *next;
} hdr_t;


void
request_handle(void *arg)
{
    int cfd = (int) arg;
     // use getpeername() to get address of peer socket

    rbuf_t rbuf;
    char buf[BUF_SIZE], method[MAX_LEN], uri[4096], proto_ver[MAX_LEN];

    readBufInit(cfd, &rbuf);
    readLineFromBuf(&rbuf, buf, BUF_SIZE);

    /* Parse request line */
    sscanf(buf, "%s %s %s", method, uri, proto_ver);

    char tmp[MAX_LEN];
    strcpy(tmp, proto_ver);
    char *proto = strtok(tmp, "/");
    char *ver = strtok(NULL, "/");

    if (strcmp(proto, "HTTP") || strcmp(ver, "1.0") || strcmp(ver, "1.1")) {
        errMsg("request_handle(): Invalid HTTP request protocol version");
        return;
    }

    switch (method) {
        case "GET":
            request_get(&rbuf, uri);
        default :
            request_error();
            errMsg("request_handle(): Invalid HTTP request method");
            return;
    }

}

void
request_get(rbuf_t rbuf_p, char *uri)
{
    char filename[4096];

    request_parse_uri(uri, filename)

    request_parse_hdr(&rbuf);

}

hdr_t **
request_parse_hdr(rbuf_t *rbuf_p)
{
    hdr_t **hdr_pp = (hdr_t **) malloc(sizeof(*hdr_pp));
    if (hdr_pp == NULL) {
        errMsg("request_parse_hdr(): Failed to allocate memory for pointer to hdr_t structures");
        return NULL:
    }

    char buf[BUF_SIZE];
    hdr_t *hdr_p = *hdr_pp;
    while (1) {
        readLineFromBuf(rbuf_p, buf, BUF_SIZE);
        if (!strcmp(buf, "\r")) {
            break;
        }

        hdr_t *tmp_p = (hdr_t *) malloc(sizeof(**hdr_pp));
        if (tmp_p == NULL) {
            errMsg("request_parse_hdr(): Failed to allocate memory for hdr_t structure");
            return NULL:
        }

        if (hdr_p == NULL) {
            hdr_p = tmp_p;
        }
        else {
            hdr_p->next = tmp_p;
            hdr_p = hdr_p->next;
        }

    }
}

void
request_parse_uri(char *uri, char *filename)
{
    struct stat stbuf;

}