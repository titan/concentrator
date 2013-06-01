#ifndef CFG_GPIO_LIB
#define CFG_GPIO_LIB
#endif
#ifndef CFG_USART_LIB
#define CFG_USART_LIB
#endif
#include"libs_emsys_odm.h"
#include <sys/select.h>
#include <unistd.h>

#define TX_ENABLE(gpio) PIOOutValue(gpio, 0)
#define RX_ENABLE(gpio) PIOOutValue(gpio, 1)

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
    if (argc != 4) {
        fprintf(stderr, "%s /dev/ttyS? BaudRate string\n", argv[0]);
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
    TX_ENABLE(gpio);
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
                TX_ENABLE(gpio);
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
