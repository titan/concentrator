#ifndef _LOGDB_H
#define _LOGDB_H
#ifdef __cplusplus
extern "C" {
#endif

#include "sbtree.h"

typedef enum {
    DB_NEW,
    DB_APPEND,
    DB_RDONLY
}
DBFLAG;

typedef sbtidx_t (* genkeyfun)(void * key, size_t len);
typedef void (* dbseqfun)(void * key, size_t klen, void * data, size_t dlen, void * params, size_t plen);

typedef struct {
    int dfd;
    int ifd;
    int pfd;
    off_t bof; // begin of file
    sbtree_t * root;
    genkeyfun genkey;
    DBFLAG flag;
} LOGDB;

LOGDB * dbopen(const char * file, DBFLAG flag, genkeyfun genkey);
void dbclose(LOGDB * db);
int dbget(LOGDB * db, void * key, size_t klen, void * data, size_t * dlen);
int dbput(LOGDB * db, void * key, size_t klen, void * data, size_t dlen);
int dbseq(LOGDB * db, dbseqfun seqfun, void * params, size_t len);

#ifdef __cplusplus
}
#endif
#endif
