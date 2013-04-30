#include "sbtree.h"

inline void sbtree_left_rotate(sbtree_t ** t)
{
  sbtree_t * r = SBTREE_RIGHT(* t);

  SBTREE_RIGHT(* t) = SBTREE_LEFT(r);
  SBTREE_LEFT(r) = * t;
  SBTREE_SIZE(r) = SBTREE_SIZE(* t);
  SBTREE_SIZE(* t) = ((!SBTREE_LEFT(* t))? 0: SBTREE_SIZE(SBTREE_LEFT(* t))) + ((!(SBTREE_RIGHT(* t)))? 0: SBTREE_SIZE(SBTREE_RIGHT(* t))) + 1;
  * t = r;
}

inline void sbtree_right_rotate(sbtree_t ** t)
{
  sbtree_t * l = SBTREE_LEFT(* t);

  SBTREE_LEFT(* t) = SBTREE_RIGHT(l);
  SBTREE_RIGHT(l) = * t;
  SBTREE_SIZE(l) = SBTREE_SIZE(* t);
  SBTREE_SIZE(* t) = ((!SBTREE_LEFT(* t))? 0: SBTREE_SIZE(SBTREE_LEFT(* t))) + ((!(SBTREE_RIGHT(* t)))? 0: SBTREE_SIZE(SBTREE_RIGHT(* t))) + 1;
  * t = l;
}

static void sbtree_maintain(sbtree_t ** t, int flag)
{
  if (!flag) {
    if (SBTREE_LEFT(* t) && SBTREE_LEFT(SBTREE_LEFT(* t))
        && (!SBTREE_RIGHT(* t) || SBTREE_SIZE(SBTREE_LEFT(SBTREE_LEFT(* t))) > SBTREE_SIZE(SBTREE_RIGHT(* t)))) {
      /* case 1 */
      sbtree_right_rotate(t);
    } else if (SBTREE_LEFT(* t) && SBTREE_RIGHT(SBTREE_LEFT(* t))
               && (!SBTREE_RIGHT(* t) || SBTREE_SIZE(SBTREE_RIGHT(SBTREE_LEFT(* t))) > SBTREE_SIZE(SBTREE_RIGHT(* t)))) {
      /* case 2 */
      sbtree_left_rotate(&SBTREE_LEFT(* t));
      sbtree_right_rotate(t);
    } else
      return;
  } else {
    if (SBTREE_RIGHT(* t) && SBTREE_RIGHT(SBTREE_RIGHT(* t))
        && (!SBTREE_LEFT(* t) || SBTREE_SIZE(SBTREE_RIGHT(SBTREE_RIGHT(* t))) > SBTREE_SIZE(SBTREE_LEFT(* t)))) {
      /* case 1' */
      sbtree_left_rotate(t);
    } else if (SBTREE_RIGHT(* t) && SBTREE_LEFT(SBTREE_RIGHT(* t))
               && (!SBTREE_LEFT(* t) || SBTREE_SIZE(SBTREE_LEFT(SBTREE_RIGHT(* t))) > SBTREE_SIZE(SBTREE_LEFT(* t)))) {
      /* case 2' */
      sbtree_right_rotate(&SBTREE_RIGHT(* t));
      sbtree_left_rotate(t);
    } else
      return;
  }
  sbtree_maintain(&SBTREE_LEFT(* t), 0);   /* fix the left subtree */
  sbtree_maintain(&SBTREE_RIGHT(* t), 1);  /* fix the right subtree */
  sbtree_maintain(t, 0);           /* fix the whole tree */
  sbtree_maintain(t, 1);           /* fix the whole tree */
}

void sbtree_insert(sbtree_t ** t, sbtree_t * node, getkeyfun getkey)
{
  if (!* t) {
    * t = node;
    SBTREE_SIZE(node) = 1;
    SBTREE_LEFT(node) = SBTREE_RIGHT(node) = 0;
  } else {
    SBTREE_SIZE(* t)++;
    sbtree_insert(getkey(node) <= getkey(* t)? &SBTREE_LEFT(* t): &SBTREE_RIGHT(* t), node, getkey);
    sbtree_maintain(t, getkey(node) > getkey(* t));
  }
}

sbtree_t * sbtree_delete(sbtree_t ** t, sbtidx_t key, getkeyfun getkey)
{
  sbtree_t * del, * node;

  SBTREE_SIZE(* t)--;

  if (key == getkey(* t) || (key < getkey(* t) && !SBTREE_LEFT(* t))
      || (key > getkey(* t) && !SBTREE_RIGHT(* t))) {
    if (!SBTREE_LEFT(* t) || !SBTREE_RIGHT(* t)) {
      del = * t;
      * t = (SBTREE_LEFT(* t))? SBTREE_LEFT(* t): SBTREE_RIGHT(* t);
    } else {
      del = sbtree_delete(&SBTREE_LEFT(* t), key + 1, getkey);
      node = * t;
      SBTREE_LEFT(del) = SBTREE_LEFT(* t);
      SBTREE_RIGHT(del) = SBTREE_RIGHT(* t);
      SBTREE_SIZE(del) = SBTREE_SIZE(* t);
      * t = del;
      del = node;
    }
    return del;
  } else {
    return sbtree_delete(key < getkey(* t)? &SBTREE_LEFT(* t): &SBTREE_RIGHT(* t), key, getkey);
  }
}

sbtree_t * sbtree_search(sbtree_t * t, sbtidx_t key, getkeyfun getkey)
{
  if (t) {
    if (getkey(t) == key)
      return t;
    else
      return sbtree_search((getkey(t) > key)? SBTREE_LEFT(t): SBTREE_RIGHT(t), key, getkey);
  }
  return 0;
}

void sbtree_sequence(sbtree_t * t, seqfun seq, void * data, size_t len)
{
  if (SBTREE_LEFT(t))
    sbtree_sequence(SBTREE_LEFT(t), seq, data, len);
  seq(t, data, len);
  if (SBTREE_RIGHT(t))
    sbtree_sequence(SBTREE_RIGHT(t), seq, data, len);
}

void sbtree_free(sbtree_t * t)
{
  if (SBTREE_LEFT(t))
    sbtree_free(SBTREE_LEFT(t));
  if (SBTREE_RIGHT(t))
    sbtree_free(SBTREE_RIGHT(t));
  free(t);
}
