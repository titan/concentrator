#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cbuffer.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

cbuffer_t cbuffer_create(int bitsize, int cellsize)
{
  void *buf = NULL;

#ifdef _KERNEL
  buf = malloc(sizeof(cbuffer_t) + cellsize * (2 << (bitsize - 1)), M_ONEWAY, M_NOWAIT);
#else
  buf = malloc(sizeof(struct _cbuffer) + cellsize * (2 << (bitsize - 1)));
  bzero(buf, sizeof(struct _cbuffer) + cellsize * (2 << (bitsize - 1)));
#endif

  if (buf) {
    ((cbuffer_t) buf)->start = (unsigned char *) buf + sizeof(struct _cbuffer);
    ((cbuffer_t) buf)->w = 0;
    ((cbuffer_t) buf)->r = 0;
    ((cbuffer_t) buf)->mask = (2 << (bitsize - 1)) - 1;
    ((cbuffer_t) buf)->cellsize = cellsize;
  }
  return (cbuffer_t) buf;
}

void cbuffer_free(cbuffer_t buf)
{
  if (buf) {
    free(buf);
  }
}

void *cbuffer_read(cbuffer_t buf)
{
  void *data = NULL;
  if (buf->r == buf->w)
    return NULL;
  data = (unsigned char *) (buf->start) + (buf->r & buf->mask) * buf->cellsize;
  return data;
}

void *cbuffer_write(cbuffer_t buf)
{
  void *data = NULL;
  if (cbuffer_count(buf) == buf->mask + 1)
    return NULL;

  data = (unsigned char *) (buf->start) + (buf->w & buf->mask) * buf->cellsize;

  return data;
}

void cbuffer_clear(cbuffer_t buf)
{
  buf->w = buf->r = 0;
}
