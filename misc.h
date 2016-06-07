#ifndef OSLAB5_MISC_H
#define OSLAB5_MISC_H

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define DEBUG

#define API_EXIT "exit"
#define API_GET_SCHEMA "get_schema"
#define API_GET_SMOKING "get_smoking"
#define API_GET_PASSENGERS "get_passengers"
#define API_BUY_TICKET "buy_ticket"

#define TOTAL_TICKETS 30
#define PORT 3490
#define MAXDATASIZE 2048 // max number of bytes we can get at once

#ifdef NDEBUG
    #define log_debug(M, ...)
#else
    #define log_debug(M, ...) {\
    fprintf(stderr, "\x1B[32m[");\
    format_time();\
    fprintf(stderr, " DEBUG]\x1B[0m (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__);\
}
#endif

#define log_info(M, ...) {\
    fprintf(stderr, "\x1B[33m[");\
    format_time();\
    fprintf(stderr, " INFO]\x1B[0m (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__);\
}

#define log_err(M, ...) {\
    fprintf(stderr, "\x1B[31m[");\
    format_time();\
    fprintf(stderr, " ERROR]\x1B[0m (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__);\
}
#define fatal_err(M, ...) {\
    fprintf(stderr, "\x1B[31m[");\
    format_time();\
    fprintf(stderr, " ERROR]\x1B[0m (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__);\
    fflush(stderr);\
    exit(EXIT_FAILURE);\
}

typedef int sockfd_t;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &((struct sockaddr_in*) sa)->sin_addr;
    }

    return &((struct sockaddr_in6*) sa)->sin6_addr;
}


void format_time(){
    time_t rawtime;
    time(&rawtime);

    struct tm *timeinfo = localtime ( &rawtime );
    struct timeval ms;
    gettimeofday(&ms, NULL);

    fprintf(stderr, "%d:%d:%d:%d",
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec,
            ms.tv_usec
    );
}

long send_msg(sockfd_t sock, char *buf, size_t bufsz) {
    long write_sz;
    if ((write_sz = write(sock, buf, bufsz)) == -1) {
        log_err("write failed");
    }
    return write_sz;
}

long get_msg(sockfd_t sock, char *buf, size_t bufsz) {
    long read_size = read(sock, buf, bufsz-1);
    buf[read_size] = '\0';

    if (!read_size) return 0;
    if (read_size < 0) log_err("client read failed");
    return read_size;
}

#endif //OSLAB5_MISC_H