#include <stdio.h>
#include <stdlib.h>
#include "logdb.h"

typedef struct {
    int id;
    int value;
} DATA;

sbtidx_t genkey(void * data, size_t len) {
    size_t i = 0;
	sbtidx_t seed = 131; // 31 131 1313 13131 131313 etc..
	sbtidx_t hash = 0;

    for (; i < len; i ++) {
		hash = hash * seed + * (unsigned char *) (data + i);
	}

	return hash;
}

void seq(void * key, size_t klen, void * data, size_t dlen, void * param, size_t plen) {
    DATA * d = (DATA *) data;
    printf("%4d - %d\n", *(int *)key, d->value);
}

int main(int argc, char ** argv) {
    DATA data;
    int i = 0, r;
    size_t len;
    LOGDB * db = NULL;


    db = dbopen("testdb", DB_NEW, genkey);
    if (db != NULL) {
        for (i = 0; i < 100; i ++) {
            data.id = i;
            data.value = i;
            dbput(db, &i, sizeof(int), &data, sizeof(DATA));
        }
        dbclose(db);
    } else {
        printf("Open test error\n");
    }

    db = dbopen("testdb", DB_APPEND, genkey);
    if (db != NULL) {
        for (i = 100; i < 200; i ++) {
            data.id = i;
            data.value = i;
            dbput(db, &i, sizeof(int), &data, sizeof(DATA));
        }
        dbclose(db);
    } else {
        printf("Open test error\n");
    }

    db = dbopen("testdb", DB_RDONLY, genkey);
    if (db != NULL) {
        r = dbseq(db, seq, NULL, 0);
        dbclose(db);
    }
}
