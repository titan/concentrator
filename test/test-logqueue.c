#include <stdio.h>
#include <stdlib.h>
#include "logqueue.h"

int main(int argc, char ** argv) {
    int i;
    unsigned int len;
    LOGQUEUE * q = lqopen("testqueue");
    printf("isempty? %d\n", lqempty(q));
    printf("start to enqueue\n");
    for (i = 0; i < 100; i ++) {
        if (!lqenqueue(q, &i, sizeof(int))) {
            printf("something wrong when enqueue %d\n", i);
        } else {
            printf("enqueue %d\n", i);
        }
    }
    if (!lqfront(q, &i, &len)) {
        printf("something wrong when get front of queue\n");
    } else {
        printf("front is %d\n", i);
    }
    printf("isempty? %d\n", lqempty(q));
    printf("start to dequeue\n");
    while (!lqempty(q)) {
        if (!lqdequeue(q, &i, &len)) {
            printf("something wrong when dequeue\n");
        } else {
            printf("dequeue %d\n", i);
        }
    }
    printf("isempty? %d\n", lqempty(q));

    lqclose(q);
    return 0;
}
