
#include "../utils/tlpi_hdr.h"
#include "../utils/utils.h"
#include "../utils/inet_sockets.h"
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define MAX_LEN 1024


/* ========================== STRUCTURES ============================ */


typedef struct hdr_t {
    char *name;
    char *value;
    struct hdr_t *next;
} hdr_t;


/* ========================== PROTOTYPES ============================ */

static void request_get(int cfd, rbuf_t rbuf_p, char *uri);
static hdr_t **request_parse_hdr(rbuf_t *rbuf_p, int cfd);
static void request_destroy_hdr(hdr_t **hdr_pp);
static void request_parse_uri(char *uri, char *filename);
static void response_get(int cfd, char *filename);
static void response_serve_static(int cfd, char *filename, int filesize);
static void response_get_content_type(char *filename, char *content_type);
static void request_error(int cfd, const char *status_code, const char *reason, const char *msg);


void
request_handle(void *arg)
{
    int cfd = (int) arg;

    rbuf_t rbuf;
    char buf[BUF_SIZE], method[MAX_LEN], uri[MAX_LEN*4], proto_ver[MAX_LEN];
    struct sockaddr my_addr;  /* Socket address buffer */
    socklen_t len = sizeof(my_addr); /* Size of socket address buffer */
    char addr_str[IS_ADDR_STR_LEN];

    /* Get peer socket address */
    if (getpeername(cfd, &my_addr, &len) == -1) {
        errMsg("request_handle(): getpeername(): Failed to get peer socket address");
    }

    readBufInit(cfd, &rbuf);
    if (readLineFromBuf(&rbuf, buf, BUF_SIZE) <= 0) {   // can use recv() sys call
        /* Connection error. Peer may have closed socket. */
        errMsg("request_handle(): readLineFromBuf(): Error reading from socket. Peer may have closed connection");
        return;
    }

    /* Parse request line */
    sscanf(buf, "%s %s %s", method, uri, proto_ver);

    /* Print peer request line */
    printf("%s %s", inetAddressStr(&my_addr, len, addr_str, IS_ADDR_STR_LEN), buf);
    fflush(stdout);

    char tmp[MAX_LEN];
    strcpy(tmp, proto_ver);
    char *proto = strtok(tmp, "/");
    char *ver = strtok(NULL, "/");

    if (strcmp(proto, "HTTP")) {
        request_error(cfd, "400", "Bad Request", "Client sent a Non-HTTP request");
        errMsg("request_handle(): Client sent a Non-HTTP request");
        return;
    }

    if (strcmp(ver, "1.0") && strcmp(ver, "1.1"))
    {
        request_error(cfd, "505", "HTTP Version Not Supported", "");
        errMsg("request_handle(): 505 HTTP Version Not Supported");
        return;
    }

    if (!strcmp(method, "GET")) {
        request_get(cfd, rbuf, uri);
    }
    else {
        request_error(cfd, "501", "Not Implemented", "Server cannot fulfill the request method for now");
        errMsg("request_handle(): Unable to fulfill HTTP request method");
        return;
    }

    close(cfd);
}

static void
request_get(int cfd, rbuf_t rbuf, char *uri)
{
    char filename[MAX_LEN*4];

    request_parse_uri(uri, filename);

    hdr_t **hdr_pp = request_parse_hdr(&rbuf, cfd);

    response_get(cfd, filename);

    request_destroy_hdr(hdr_pp);
}

static hdr_t **
request_parse_hdr(rbuf_t *rbuf_p, int cfd)
{   // Return 500 Internal Server Error for failed mallocs?
    hdr_t **hdr_pp = (hdr_t **) malloc(sizeof(*hdr_pp));
    if (hdr_pp == NULL) {
        errMsg("request_parse_hdr(): Failed to allocate memory for pointer to hdr_t structures");
        return NULL;
    }

    char buf[BUF_SIZE];
    hdr_t *hdr_p = *hdr_pp;
    int empty_list = 1;
    while (1) {
        if (readLineFromBuf(rbuf_p, buf, BUF_SIZE) <= 0) {  // can use recv() sys call
            errMsg("request_handle(): readLineFromBuf(): Error reading from socket. Peer may have closed connection");
            return hdr_pp;
        }

        if (!strcmp(buf, "\r\n")) {
            break;
        }

        char *name = trimwhitespace(strtok(buf, ":"));
        char *value = trimwhitespace(strtok(NULL, ":"));

        if (name == NULL || value == NULL) {
            errMsg("request_parse_hdr(): Bad header field format");
            continue;
        }

        hdr_t *tmp_p = (hdr_t *) malloc(sizeof(*tmp_p));
        if (tmp_p == NULL) {
            errMsg("request_parse_hdr(): Failed to allocate memory for hdr_t structure");
            continue;
        }
        tmp_p->next = NULL; // Initialize next member

        if (empty_list) {
            *hdr_pp = tmp_p;
            hdr_p = tmp_p;
            empty_list = 0;
        }
        else {
            hdr_p->next = tmp_p;
            hdr_p = hdr_p->next;
        }

        hdr_p->name = (char *) malloc((strlen(name)+1)*sizeof(char));
        if (hdr_p->name == NULL) {
            errMsg("request_parse_hdr(): Failed to allocate memory for hdr name");
        }
        else {
            strcpy(hdr_p->name, name);
        }

        hdr_p->value = (char *) malloc((strlen(value)+1)*sizeof(char));
        if (hdr_p->value == NULL) {
            errMsg("request_parse_hdr(): Failed to allocate memory for hdr value");
        }
        else {
            strcpy(hdr_p->value, value);
        }

        buf[0] = '\0';
    }

    return hdr_pp;
}

static void
request_destroy_hdr(hdr_t **hdr_pp)
{
    if (hdr_pp == NULL)
        return;

    hdr_t *hdr_p = *hdr_pp, *next;
    while (hdr_p != NULL) {
        next = hdr_p->next;
        free(hdr_p->name);
        free(hdr_p->value);
        free(hdr_p);
        hdr_p = next;
    }

    free(hdr_pp);
}

static void
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

static void
response_get(int cfd, char *filename)
{
    struct stat sbuf;
    if (stat(filename, &sbuf) == -1) {
        request_error(cfd, "404", "Not Found", "The requested resource could not be found");
        return;
    }

    if (!(S_ISREG(sbuf.st_mode) && (sbuf.st_mode & S_IRUSR))) {
        request_error(cfd, "403", "Forbidden", "");
        return;
    }

    response_serve_static(cfd, filename, sbuf.st_size);
}

static void
response_serve_static(int cfd, char *filename, int filesize)
{
    char resp[BUF_SIZE];//, hdr[BUF_SIZE]; // write a function that creates the header?
    char content_type[MAX_LEN];
    response_get_content_type(filename, content_type);

    // Header
    sprintf(resp, "HTTP/1.0 200 OK\r\n");
    sprintf(resp, "%sServer: Tzou's HTTP server\r\n", resp);
    sprintf(resp, "%sContent-Type: %s\r\n", resp, content_type);
    sprintf(resp, "%sContent-Length: %d\r\n", resp, filesize);
    strcat(resp, "\r\n");
    if (writen(cfd, resp, strlen(resp)) == -1) {    // can use send() sys call
        errMsg("response_serve_static(): writen(): Failed to write headers to socket. Peer may have closed connection.");
        return;
    }

    // Body
    int in_fd;
    if ((in_fd = open(filename, O_RDONLY)) < 0) {
        errMsg("Failed to open file %s", filename);
        request_error(cfd, "500", "Internal Server Error", "");
        return;
    }
    off_t offset = 0;
    ssize_t nbytes = sendfile(cfd, in_fd, &offset, filesize);   // Can use mmap(). TCP_CORK option?
    if (nbytes < 0) {
        errMsg("response_serve_static(): sendfile(): Failed to send file to socket");
        return;
    }
}

static void
response_get_content_type(char *filename, char *content_type)
{
    const char *ext;
    ext = get_filename_ext(filename);

    if (!strcmp(ext, "html")) {
        sprintf(content_type, "text/html");
    }
    else if (!(strcmp(ext, "jpeg") || strcmp(ext, "jpg"))) {
        sprintf(content_type, "image/jpeg");
    }
    else {
        sprintf(content_type, "text/plain");
    }
}

static void
request_error(int cfd, const char *status_code, const char *reason, const char *msg)
{   // Serve static webpage for each error code?
    char body[MAX_LEN];
    sprintf(body, "<!DOCTYPE html><html lang=\"en\"><head><title>Error page</title></head>");
    sprintf(body, "%s<body><h1><b>%s %s</b></h1>", body, status_code, reason);
    sprintf(body, "%s<p>%s</p></body></html>", body, msg);

    char resp[BUF_SIZE];
    sprintf(resp, "HTTP/1.0 %s %s\r\n", status_code, reason);
    sprintf(resp, "%sServer: Tzou's HTTP server\r\n", resp);
    sprintf(resp, "%sContent-Type: %s\r\n", resp, "text/html");
    sprintf(resp, "%sContent-Length: %lu\r\n", resp, strlen(body));
    strcat(resp, "\r\n");
    strcat(resp, body);

    if (writen(cfd, resp, strlen(resp)) == -1) {    // can use send(). handle error retval of -1.
        errMsg("request_error(): writen(): Failed to write to socket. Peer may have closed connection.");
        return;
    }
}
