#include <stdio.h>
#include <time.h>
#include <errno.h>

int myusleep (unsigned long usec) {
    struct timespec tv;
    tv.tv_sec = usec / 1000000;
    tv.tv_nsec = (usec % 1000000) * 1000 ;

    while (1) {
        int rval = nanosleep(&tv, &tv);
        if (rval == 0)
            return 0;
        else if (errno == EINTR)
            continue;
        else
            return rval;
    }
    return 0;
}

int main(int argc, char ** argv) {
    unsigned long t = 500;
    if (argc > 1) {
        sscanf(argv[1], "%ld", &t);
    }
    printf("delay %d ms\n", t);
    printf("start\n");
    myusleep(t * 1000);
    printf("end\n");
    return 0;
}
