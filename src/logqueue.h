#ifndef _LOG_QUEUE_H
#define _LOG_QUEUE_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    int dfd; // data file
    int ifd; // index file
    int pfd; // pointer file
} LOGQUEUE;

typedef struct {
    off_t offset;
    size_t length;
} LQIDX;

LOGQUEUE * lqopen(const char * name);
void lqclose(LOGQUEUE * q);
int lqempty(LOGQUEUE * q);
int lqenqueue(LOGQUEUE * q, void * data, size_t len);
int lqdequeue(LOGQUEUE * q, void * data, size_t * len);
int lqfront(LOGQUEUE * q, void * data, size_t * len);
#ifdef __cplusplus
}
#endif
#endif
