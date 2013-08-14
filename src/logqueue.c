#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "logqueue.h"

typedef struct {
    unsigned int offset;
    unsigned int length;
} LQIDX;

LOGQUEUE * lqopen(const char * name) {
    char df[1024], idf[1204], pf[1024];
    LOGQUEUE * q = (LOGQUEUE *) malloc(sizeof(LOGQUEUE));
    unsigned int offset;
    if (q != NULL) {
        bzero(q, sizeof(LOGQUEUE));
        bzero(df, 1024);
        bzero(idf, 1024);
        bzero(pf, 1024);
        sprintf(df, "%s.dat", name);
        sprintf(idf, "%s.idx", name);
        sprintf(pf, "%s.ptr", name);
        q->dfd = open(df, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        q->ifd = open(idf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        q->pfd = open(pf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (q->pfd != -1 && q->ifd != -1) {
            if (-1 != lseek(q->pfd, 0 - sizeof(unsigned int), SEEK_END)) {
                if (read(q->pfd, &offset, sizeof(unsigned int)) > 0) {
                    lseek(q->ifd, offset, SEEK_SET);
                }
            }
        }
        if (q->dfd == -1 || q->ifd == -1 || q->pfd == -1) {
            if (q->dfd != -1) {
                close(q->dfd);
            }
            if (q->ifd != -1) {
                close(q->ifd);
            }
            if (q->pfd != -1) {
                close(q->pfd);
            }
        }
    }
    return q;
}

void lqclose(LOGQUEUE * q) {
    close(q->dfd);
    close(q->ifd);
    close(q->pfd);
    free(q);
}

int lqempty(LOGQUEUE * q) {
    unsigned int offset;
    if (lseek(q->pfd, 0 - sizeof(unsigned int), SEEK_END) == -1) {
        offset = 0;
    } else if (read(q->pfd, &offset, sizeof(unsigned int)) != sizeof(unsigned int)) {
        offset = 0;
    }
    if (lseek(q->ifd, 0, SEEK_END) == offset) {
        return 1; // true
    } else {
        lseek(q->ifd, offset, SEEK_SET);
        return 0; // false
    }
}

int lqenqueue(LOGQUEUE * q, void * data, unsigned int len) {
    LQIDX idx;
    idx.offset = lseek(q->dfd, 0, SEEK_END);
    idx.length = len;
    if (write(q->dfd, data, len) == (ssize_t)len) {
        lseek(q->ifd, 0, SEEK_END);
        if (write(q->ifd, &idx, sizeof(LQIDX)) == sizeof(LQIDX)) {
            return 1; // true
        }
        return 0; // false;
    }
    return 0; // false
}

int lqdequeue(LOGQUEUE * q, void * data, unsigned int * len) {
    LQIDX idx;
    unsigned int offset;
    if (lseek(q->pfd, 0 - sizeof(unsigned int), SEEK_END) == -1) {
        offset = 0;
    } else if (read(q->pfd, &offset, sizeof(unsigned int)) != sizeof(unsigned int)) {
        return 0; // false
    }
    if (lseek(q->ifd, offset, SEEK_SET) != -1) {
        if (read(q->ifd, &idx, sizeof(LQIDX)) == sizeof(LQIDX)) {
            if (lseek(q->dfd, idx.offset, SEEK_SET) != -1) {
                if (read(q->dfd, data, idx.length) == (ssize_t)idx.length) {
                    lseek(q->pfd, 0, SEEK_END);
                    offset = lseek(q->ifd, 0, SEEK_CUR);
                    if (offset != (unsigned int)-1) {
                        if (write(q->pfd, &offset, sizeof(unsigned int)) == sizeof(unsigned int)) {
                            * len = idx.length;
                            return 1;
                        }
                        return 0; // false
                    }
                    return 0; // false
                }
                return 0; // false
            }
            return 0; // false
        }
        return 0; // false
    }
    return 0; // false
}

int lqfront(LOGQUEUE * q, void * data, unsigned int * len) {
    LQIDX idx;
    unsigned int offset;
    if (lseek(q->pfd, 0 - sizeof(unsigned int), SEEK_END) == -1) {
        offset = 0;
    } else if (read(q->pfd, &offset, sizeof(unsigned int)) != sizeof(unsigned int)) {
        return 0; // false
    }
    if (lseek(q->ifd, offset, SEEK_SET) != -1) {
        if (read(q->ifd, &idx, sizeof(LQIDX)) == sizeof(LQIDX)) {
            if (lseek(q->dfd, idx.offset, SEEK_SET) != -1) {
                if (read(q->dfd, data, idx.length) == (ssize_t)idx.length) {
                    * len = idx.length;
                    return 1;
                }
                return 0; // false
            }
            return 0; // false
        }
        return 0; // false
    }
    return 0; // false
}
