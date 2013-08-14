#ifndef _SBTREE_H
#define _SBTREE_H

#include <stdlib.h>

#define SBTREE_LEFT(t) ((t)->left)
#define SBTREE_RIGHT(t) ((t)->right)
#define SBTREE_SIZE(t) ((t)->size)

typedef struct _sbtree {
    struct _sbtree *left;
    struct _sbtree *right;
    size_t size;
} sbtree_t;

typedef size_t sbtidx_t;
typedef sbtidx_t (* getkeyfun)(const void *);
typedef void (* seqfun)(sbtree_t *, void *, size_t);

void sbtree_insert(sbtree_t **, sbtree_t *, getkeyfun);
sbtree_t * sbtree_delete(sbtree_t **, sbtidx_t, getkeyfun);
sbtree_t * sbtree_search(sbtree_t *, sbtidx_t, getkeyfun);
void sbtree_sequence(sbtree_t *, seqfun, void *, size_t);
void sbtree_free(sbtree_t *);
#endif
