#ifndef _LOGDB_H
#define _LOGDB_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DB_NEW,
    DB_APPEND,
    DB_RDONLY
} DBFLAG;

typedef enum {
    R_FIRST,
    R_LAST,
    R_NEXT,
    R_PREV
} SEQFLAG;

typedef struct {
    int dfd;
    int gfd;
    off_t bof; // begin of file
    size_t rlen;
    DBFLAG flag;
} LOGDB;

LOGDB * dbopen(const char * file, DBFLAG flag, size_t rlen);
void dbclose(LOGDB * db);
int dbput(LOGDB * db, void * data, size_t len);
int dbseq(LOGDB * db, void * data, SEQFLAG flag);

#ifdef __cplusplus
}
#endif
#endif
