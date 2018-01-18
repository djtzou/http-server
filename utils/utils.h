/* utils.h

   Header file for utils.c
*/

#ifndef UTILS_H
#define UTILS_H

ssize_t readLine(int fd, void *buffer, size_t n);

ssize_t readn(int fd, void *buffer, size_t n);

ssize_t writen(int fd, const void *buffer, size_t n);

/* Bookkeeping data structure containing an intermediary
 * userspace buffer for buffered reads so that we can avoid
 * multiple read() system calls */
#define BUF_SIZE 8192       /* Specify the (block) size of our intermediary buffer */
typedef struct {
    int fd;                 /* File descriptor of the I/O resource to read from */
    int cnt;                /* Unread bytes in the intermediary buffer */
    char *bufptr;           /* Next unread byte in the intermediary buffer */
    char buf[BUF_SIZE];     /* Intermediary userspace buffer */
} rbuf_t;

void readBufInit(int fd, rbuf_t *rb);

ssize_t readLineFromBuf(rbuf_t *rb, void *buffer, size_t n);

ssize_t readnFromBuf(rbuf_t *rb, void *buffer, size_t n);


/* Default file permissions are DEF_MODE & ~DEF_UMASK */
#define DEF_MODE   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
#define DEF_UMASK  S_IWGRP|S_IWOTH


char *trimwhitespace(char *str);

size_t trimwhitespace(char *out, size_t len, const char *str);


#endif
