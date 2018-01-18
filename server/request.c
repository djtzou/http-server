
#include "../utils/utils.h"
#include <sys/stat.h>

#define MAX_LEN 1024


typedef struct hdr_t {
    char *name;
    char *value;
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
            request_get(cfd, &rbuf, uri);
        default :
            //TODO: request_error(cfd, );
            errMsg("request_handle(): Invalid HTTP request method");
            return;
    }
}

void
request_get(int cfd, rbuf_t rbuf_p, char *uri)
{
    char filename[4096];

    request_parse_uri(uri, filename);

    hdr_t **hdr_pp = request_parse_hdr(&rbuf);

    response_get(cfd, filename);
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
        if (!strcmp(buf, "\r\n")) {
            break;
        }

        char *name = trimwhitespace(strtok(buf, ":"));
        char *value = trimwhitespace(strtok(NULL, ":"));

        if (name == NULL || value == NULL) {
            errMsg("request_parse_hdr(): Bad header field format");
            continue;
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

        hdr_p->name = (char *) malloc(sizeof(strlen(name))+1);
        strcpy(hdr_p->name, name);
        hdr_p->value = (char *) malloc(sizeof(strlen(value))+1);
        strcpy(hdr_p->value, value);

        buf[0] = '\0';
    }

    return hdr_pp;
}

void
request_destroy_hdr(hdr_t **hdr_pp)
{
    hdr_t *hdr_p, *next;
    hdr_p = *hdr_pp;
    while (hdr_p != NULL) {
        next = hdr_p->next;
        free(hdr_p->name);
        free(hdr_p->value);
        free(hdr_p);
        hdr_p = next;
    }

    free(hdr_pp);
}

void
request_parse_uri(char *uri, char *filename)
{
    if (sprintf(filename, ".%s", uri) < 0) {
        errMsg("request_parse_uri(): sprintf() failed");
        return;
    }

    if (uri[strlen(uri)-1] == '/') {
        strcat(filename, "index.html");
    }
}

void
response_get(int cfd, char *filename)
{
    struct stat sbuf;
    if (stat(filename, &sbuf) == -1) {
        //TODO: request_error(cfd, );
        return;
    }

    if (!(S_ISREG(sbuf.st_mode) && (sbuf.st_mode & S_IRUSR)))
        //TODO: request_error(cfd, );


    response_serve_static();
}


request_error()
request_get_file_type()
response_serve_static()