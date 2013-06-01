#ifndef CFG_GPIO_LIB
#define CFG_GPIO_LIB
#endif
#ifndef CFG_USART_LIB
#define CFG_USART_LIB
#endif
#include "libs_emsys_odm.h"
#include <sys/select.h>
#include <unistd.h>

#define TX_ENABLE(gpio) PIOOutValue(gpio, 0)
#define RX_ENABLE(gpio) PIOOutValue(gpio, 1)

#ifndef hexdump
#define hexdump(data, len) do {int i; for (i = 0; i < (int)len; i ++) { printf("%02x ", *(unsigned char *)(data + i));} printf("\n");} while(0)
#endif

inline gpio_name_t getGPIO(const char * device) {
    if (strcmp(device, "/dev/ttyS4") == 0) {
        printf("using GPIO: PA18\n");
        return PA18;
    } else if (strcmp(device, "/dev/ttyS5") == 0) {
        printf("using GPIO: PA19\n");
        return PA19;
    } else if (strcmp(device, "/dev/ttyS6") == 0) {
        printf("using GPIO: PA20\n");
        return PA20;
    } else if (strcmp(device, "/dev/ttyS7") == 0) {
        printf("using GPIO: PA21\n");
        return PA21;
    } else {
        printf("using default GPIO: PA18\n");
        return PA18;
    }
}

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
    gpio_name_t gpio = getGPIO(argv[1]);
    int fd = OpenCom(argv[1], argv[2], O_RDWR | O_NOCTTY);
    gpio_attr_t AttrOut;
    AttrOut.mode = PIO_MODE_OUT;
    AttrOut.resis = PIO_RESISTOR_DOWN;
    AttrOut.filter = PIO_FILTER_NOEFFECT;
    AttrOut.multer = PIO_MULDRIVER_NOEFFECT;
    SetPIOCfg(gpio, AttrOut);
    fd_set wfds, rfds;
    struct timeval tv;
    char buf[1024];
    int r = 0;
    int flag = 0;
    while (1) {
        FD_ZERO(&wfds);
        if (flag % 2 == 1) {
            FD_SET(fd, &wfds);
            TX_ENABLE(gpio);
        }
        FD_ZERO(&rfds);
        if (flag % 2 == 0) {
            FD_SET(fd, &rfds);
            RX_ENABLE(gpio);
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
