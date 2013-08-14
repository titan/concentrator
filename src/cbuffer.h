#ifndef _CIRCULAR_BUFFER
#define _CIRCULAR_BUFFER
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cbuffer {
    void *start;                  /* the start address of buffer */
    unsigned int w;               /* the position of the write pointer */
    unsigned int r;               /* the position of the read pointer */
    unsigned int mask;
    unsigned int cellsize;
} *cbuffer_t;

#define cbuffer_count(buf) ((buf)->w - (buf)->r)

#define cbuffer_read_done(buf) do { \
  buf->r ++; \
} while(0)

#define cbuffer_write_done(buf) do { \
  buf->w ++; \
} while(0)

cbuffer_t cbuffer_create(int bitsize, int cellsize);
void cbuffer_free(cbuffer_t buf);
void *cbuffer_read(cbuffer_t buf);
void *cbuffer_write(cbuffer_t buf);
void cbuffer_clear(cbuffer_t buf);
#ifdef __cplusplus
}
#endif
#endif
