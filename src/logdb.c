#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "logdb.h"

typedef struct {
    sbtidx_t key; // used by sbtree
    off_t offset; // offset in data file
    size_t klen; // length of key
    size_t dlen; // lenght of data
} LOGDBIDX;

typedef struct {
    sbtree_t tree; // used by sbtree
    LOGDBIDX idx;
} SBTIDX;

typedef struct {
    dbseqfun fun;
    int fd;
    void * params;
    size_t len;
} seqtuple_t;

void sequence(sbtree_t * t, void * d, size_t dlen) {
    void * key, * data;
    SBTIDX * idx = (SBTIDX *) t;
    seqtuple_t * st = (seqtuple_t *) d;
    key = malloc(idx->idx.klen);
    if (key == NULL) {
        return;
    }
    data = malloc(idx->idx.dlen);
    if (data == NULL) {
        goto QUIT1;
    }
    lseek(st->fd, idx->idx.offset, SEEK_SET);
    if ((size_t)read(st->fd, key, idx->idx.klen) != idx->idx.klen) {
        goto QUIT2;
    }
    if ((size_t)read(st->fd, data, idx->idx.dlen) != idx->idx.dlen) {
        goto QUIT2;
    }
    st->fun(key, idx->idx.klen, data, idx->idx.dlen, st->params, st->len);
QUIT2:
    free(data);
QUIT1:
    free(key);

    return;
}

sbtidx_t getkey(const void * data) {
    SBTIDX * idx = (SBTIDX *) data;
    return idx->idx.key;
}

LOGDB * dbopen(const char * name, DBFLAG flag, genkeyfun genkey) {
    char df[1024], idf[1204], pf[1024];
    LOGDBIDX logidx;
    SBTIDX * sbtidx;
    LOGDB * db = (LOGDB *) malloc(sizeof(LOGDB));
    if (db != NULL) {
        bzero(db, sizeof(LOGDB));
        bzero(df, 1024);
        bzero(idf, 1024);
        bzero(pf, 1024);
        sprintf(df, "%s.dat", name);
        sprintf(idf, "%s.idx", name);
        sprintf(pf, "%s.ptr", name);
        switch (flag) {
        case DB_NEW:
            db->dfd = open(df, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            db->ifd = open(idf, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            db->pfd = open(pf, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (db->dfd != -1 && db->ifd != -1 && db->pfd != -1) {
                db->bof = lseek(db->ifd, 0, SEEK_END);
                write(db->pfd, &db->bof, sizeof(off_t));
            }
            break;
        case DB_APPEND:
            db->dfd = open(df, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            db->ifd = open(idf, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            db->pfd = open(pf, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (db->dfd != -1 && db->ifd != -1 && db->pfd != -1) {
                lseek(db->pfd, 0 - sizeof(off_t), SEEK_END);
                read(db->pfd, &db->bof, sizeof(off_t));
                lseek(db->ifd, db->bof, SEEK_SET);
                while (read(db->ifd, &logidx, sizeof(LOGDBIDX)) > 0) {
                    sbtidx = (SBTIDX *) malloc(sizeof(SBTIDX));
                    if (sbtidx != NULL) {
                        memcpy(&sbtidx->idx, &logidx, sizeof(LOGDBIDX));
                        sbtree_insert(&db->root, (sbtree_t *)sbtidx, getkey);
                    }
                }
            }
            break;
        case DB_RDONLY:
            db->dfd = open(df, O_RDONLY);
            db->ifd = open(idf, O_RDONLY);
            db->pfd = open(pf, O_RDONLY);
            if (db->dfd != -1 && db->ifd != -1 && db->pfd != -1) {
                lseek(db->pfd, 0 - sizeof(off_t), SEEK_END);
                read(db->pfd, &db->bof, sizeof(off_t));
                lseek(db->ifd, db->bof, SEEK_SET);
                while (read(db->ifd, &logidx, sizeof(LOGDBIDX)) > 0) {
                    sbtidx = (SBTIDX *) malloc(sizeof(SBTIDX));
                    if (sbtidx != NULL) {
                        memcpy(&sbtidx->idx, &logidx, sizeof(LOGDBIDX));
                        sbtree_insert(&db->root, (sbtree_t *)sbtidx, getkey);
                    }
                }
            }
            break;
        default:
            db->dfd = -1;
            db->ifd = -1;
            db->pfd = -1;
            break;
        }
        db->genkey = genkey;
        db->flag = flag;
        if (db->dfd == -1 || db->ifd == -1 || db->pfd == -1) {
            if (db->dfd != -1) {
                close(db->dfd);
            } else if (db->ifd != -1) {
                close(db->ifd);
            } else if (db->pfd != -1) {
                close(db->pfd);
            }
            free(db);
            db = NULL;
        }
    }
    return db;
}

void dbclose(LOGDB * db) {
    close(db->dfd);
    close(db->ifd);
    close(db->pfd);
    if (db->root)
        sbtree_free(db->root);
    free(db);
}

int dbget(LOGDB * db, void * key, size_t klen, void * data, size_t * dlen) {
    sbtidx_t k = db->genkey(key, klen);
    SBTIDX * idx = (SBTIDX *) sbtree_search(db->root, k, getkey);
    if (idx == NULL)
        return -1;
    lseek(db->dfd, idx->idx.offset + idx->idx.klen, SEEK_SET);
    * dlen = read(db->ifd, data, idx->idx.dlen);
    if (* (ssize_t *) dlen == -1)
        return -1;
    return 0;
}

int dbput(LOGDB * db, void * key, size_t klen, void * data, size_t dlen) {
    SBTIDX * idx = (SBTIDX *) malloc(sizeof(SBTIDX));
    if (idx == NULL) {
        return -1;
    }
    bzero(idx, sizeof(SBTIDX));
    idx->idx.offset = lseek(db->dfd, 0, SEEK_END);
    if (idx->idx.offset == -1)
        idx->idx.offset = 0;
    if (write(db->dfd, key, klen) == -1)
        return -1;
    if (write(db->dfd, data, dlen) == -1)
        return -1;
    idx->idx.klen = klen;
    idx->idx.dlen = dlen;
    idx->idx.key = db->genkey(key, klen);
    lseek(db->ifd, 0, SEEK_END);
    write(db->ifd, &idx->idx, sizeof(LOGDBIDX));
    sbtree_insert(&db->root, (sbtree_t *)idx, getkey);
    return 0;
}

int dbseq(LOGDB * db, dbseqfun seqfun, void * params, size_t len) {
    seqtuple_t tuple;
    if (db->flag != DB_RDONLY) {
        return -1;
    }

    if (db->root) {
        tuple.fun = seqfun;
        tuple.fd = db->dfd;
        tuple.params = params;
        tuple.len = len;
        sbtree_sequence(db->root, sequence, &tuple, sizeof(seqtuple_t));
    }
    return 0;
}
