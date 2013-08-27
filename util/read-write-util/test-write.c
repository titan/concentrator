#ifndef CFG_USART_LIB
#define CFG_USART_LIB
#endif
#include"libs_emsys_odm.h"
#include <sys/select.h>
#include <unistd.h>

void delay(int ms) {
    struct timespec rqtp;
    rqtp.tv_sec = 0;
    rqtp.tv_nsec = ms * 1000 * 1000;      //1000 ns = 1 us, 1000 us = 1ms
    nanosleep(&rqtp, NULL);
}

int main(int argc, char ** argv) {
    if (argc != 4) {
        fprintf(stderr, "%s /dev/ttyS? BaudRate string\n", argv[0]);
        return -1;
    }
    fd_set wfds;
    struct timeval tv;
    while (1) {
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);

        tv.tv_sec = 5;
        tv.tv_usec = 0;
        int retval = select(fd + 1, NULL, &wfds, NULL, &tv);
        if (retval) {
            if (FD_ISSET(fd, &wfds)) {
                delay(1000000);
                sleep(1);
                write(fd, argv[3], strlen(argv[3]));
                printf("write: %s\n", argv[3]);
                //delay(50);
                //RX_ENABLE(PA20);
            }
        } else {
            fprintf(stderr, "cannot write\n");
        }
    }

    return 0;
}
