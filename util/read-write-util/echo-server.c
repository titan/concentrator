#ifndef CFG_USART_LIB
#define CFG_USART_LIB
#endif
#include "libs_emsys_odm.h"
#include <sys/select.h>
#include <unistd.h>

#ifndef hexdump
#define hexdump(data, len) do {int i; for (i = 0; i < (int)len; i ++) { printf("%02x ", *(unsigned char *)(data + i));} printf("\n");} while(0)
#endif

void delay(int ms) {
    struct timespec rqtp;
    rqtp.tv_sec = 0;
    rqtp.tv_nsec = ms * 1000 * 1000;      //1000 ns = 1 us, 1000 us = 1ms
    nanosleep(&rqtp, NULL);
}

int main(int argc, char ** argv) {
    if (argc != 3) {
        fprintf(stderr, "%s /dev/ttyS? BaudRate\n", argv[0]);
        return -1;
    }
    int fd = OpenCom(argv[1], argv[2], O_RDWR | O_NOCTTY);
    fd_set wfds, rfds;
    struct timeval tv;
    char buf[1024];
    int r = 0;
    int flag = 0;
    while (1) {
        FD_ZERO(&wfds);
        if (flag % 2 == 1) {
            FD_SET(fd, &wfds);
        }
        FD_ZERO(&rfds);
        if (flag % 2 == 0) {
            FD_SET(fd, &rfds);
        }

        tv.tv_sec = 5;
        tv.tv_usec = 0;
        int retval = select(fd + 1, &rfds, &wfds, NULL, &tv);
        if (retval) {
            if (FD_ISSET(fd, &wfds)) {
                if (r > 0) {
                    int i = 0, j = r / 2, k = r - 1;
                    for (i = 0; i < j; i ++) {
                        char c = buf[i];
                        buf[i] = buf[k - i];
                        buf[k - i] = c;
                    }
                    write(fd, buf, r);
                    printf("sent: %s\n", buf);
                    sleep(1);
                    flag ++;
                }
            }
            if (FD_ISSET(fd, &rfds)) {
                bzero(buf, 1024);
                r = read(fd, buf, 1024);
                if (r > 0) {
                    printf("recv: %s\n", buf);
                    sleep(1);
                    flag ++;
                } else {
                    printf("nothing to read\n");
                }
            }
        } else {
            fprintf(stderr, "select timeout\n");
        }
    }

    return 0;
}
