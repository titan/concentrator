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

LOGQUEUE * lqopen(const char * name);
void lqclose(LOGQUEUE * q);
int lqempty(LOGQUEUE * q);
int lqenqueue(LOGQUEUE * q, void * data, unsigned int len);
int lqdequeue(LOGQUEUE * q, void * data, unsigned int * len);
int lqfront(LOGQUEUE * q, void * data, unsigned int * len);
#ifdef __cplusplus
}
#endif
#endif
