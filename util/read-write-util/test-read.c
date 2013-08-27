#ifndef CFG_USART_LIB
#define CFG_USART_LIB
#endif
#include"libs_emsys_odm.h"
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
    unsigned char buf[1024];
    int r;
    fd_set rfds;
    struct timeval tv;
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        tv.tv_sec = 5;
        tv.tv_usec = 0;
        int retval = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (retval) {
            if (FD_ISSET(fd, &rfds)) {
                bzero(buf, 1024);
                r = read(fd, buf, 1024);
                if (r > 0) {
                    printf("read: %s\n", buf);
                } else {
                    printf("nothing to read\n");
                }
            }
        } else {
            fprintf(stderr, "read timeout\n");
        }
    }

    return 0;
}
