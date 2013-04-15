#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "logdb.h"

LOGDB * dbopen(const char * file, DBFLAG flag, size_t rlen) {
    char buf[1024];
    LOGDB * db = NULL;
    db = malloc(sizeof(LOGDB));
    if (db != NULL) {
        bzero(db, sizeof(LOGDB));
        bzero(buf, 1024);
        sprintf(buf, "%s.gp", file);
        switch (flag) {
        case DB_NEW:
            db->dfd = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            db->gfd = open(buf, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            db->bof = lseek(db->dfd, 0, SEEK_END);
            write(db->gfd, &db->bof, sizeof(off_t));
            break;
        case DB_APPEND:
            db->dfd = open(file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            db->gfd = open(buf, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            read(db->gfd, &db->bof, sizeof(off_t));
            break;
        case DB_RDONLY:
            db->dfd = open(file, O_RDONLY);
            db->gfd = open(buf, O_RDONLY);
            lseek(db->gfd, 0 - sizeof(off_t), SEEK_END);
            read(db->gfd, &db->bof, sizeof(off_t));
            lseek(db->dfd, db->bof, SEEK_SET);
            break;
        default:
            db->dfd = -1;
            db->gfd = -1;
            break;
        }
        db->flag = flag;
        db->rlen = rlen;
        if (db->dfd == -1 || db->gfd == -1) {
            if (db->dfd != -1) {
                close(db->dfd);
            } else if (db->gfd != -1) {
                close(db->gfd);
            }
            free(db);
            db = NULL;
        }
    }
    return db;
}

void dbclose(LOGDB * db) {
    close(db->dfd);
    close(db->gfd);
    free(db);
}

int dbput(LOGDB * db, void * data, size_t len) {
    lseek(db->dfd, 0, SEEK_END);
    return write(db->dfd, data, len);
}

int dbseq(LOGDB * db, void * data, SEQFLAG flag) {
    size_t r = 0;
    off_t pos = 0;
    if (db->flag != DB_RDONLY) {
        return -1;
    }
    switch (flag) {
    case R_FIRST:
        if (lseek(db->dfd, db->bof, SEEK_SET) != -1)
            r = read(db->dfd, data, db->rlen);
        else
            return -1;
        break;
    case R_LAST:
        if (lseek(db->dfd, 0 - db->rlen, SEEK_END) != -1)
            r = read(db->dfd, data, db->rlen);
        else
            return -1;
        break;
    case R_NEXT:
        r = read(db->dfd, data, db->rlen);
        break;
    case R_PREV:
        pos = lseek(db->dfd, 0, SEEK_CUR);
        if (pos - (off_t)db->rlen == db->bof)
            return -1;
        if (lseek(db->dfd, 0 - 2 * db->rlen, SEEK_CUR) != -1)
            r = read(db->dfd, data, db->rlen);
        else
            return -1;
        break;
    }
    return r;
}

#if 0

typedef struct {
    int id;
    int value;
} DATA;

int main(int argc, char ** argv) {
    DATA data;
    int i = 0, r;
    size_t len;
    /*
    LOGDB * db = dbopen("test", DB_NEW, sizeof(DATA));
    if (db != NULL) {
        for (i = 0; i < 100; i ++) {
            data.id = i;
            data.value = i;
            dbput(db, &data, sizeof(DATA));
        }
        dbclose(db);
    } else {
        printf("Open test error\n");
    }
    */
    /*
    LOGDB * db = dbopen("test", DB_APPEND, sizeof(DATA));
    if (db != NULL) {
        for (i = 100; i < 200; i ++) {
            data.id = i;
            data.value = i;
            dbput(db, &data, sizeof(DATA));
        }
        dbclose(db);
    } else {
        printf("Open test error\n");
    }
    */
    /*
    LOGDB * db = dbopen("test", DB_RDONLY, sizeof(DATA));
    if (db != NULL) {
        r = dbseq(db, &data, R_LAST);
        printf("%d %d\n", r, data.id);
        while (r > 0) {
            r = dbseq(db, &data, R_PREV);
            if (r > 0) {
                printf("%d %d\n", r, data.id);
            }
        }

        r = dbseq(db, &data, R_FIRST);
        printf("%d %d\n", r, data.id);
        while (r > 0) {
            r = dbseq(db, &data, R_NEXT);
            if (r > 0) {
                printf("%d %d\n", r, data.id);
            }
        }
        dbclose(db);
    }
    */
}

#endif
