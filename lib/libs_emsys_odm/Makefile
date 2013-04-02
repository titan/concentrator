# 指定编译器设置
include ../../script/makefile.include
CC=$(BASE)arm-none-linux-gnueabi-gcc
CFLAGS=-fPIC -g -O2
STRIP=$(BASE)arm-none-linux-gnueabi-strip
AR=$(BASE)arm-none-linux-gnueabi-ar
EXCFLAGS:=
EXTERLIBS:=

#包含配置文件
include config.mk

# Add cpu kinds to CFLAGS
ifeq ($(CPUARCH),EMSYS_AT91SAM9263)
	EXCFLAGS += -march=armv5t
	CFLAGS += $(EXCFLAGS) -DEMSYS_AT91SAM9263
#
else ifeq ($(CPUARCH),EMSYS_AT91SAM9260)
	EXCFLAGS += -march=armv5t
	CFLAGS += $(EXCFLAGS) -DEMSYS_AT91SAM9260
#
else
	EXCFLAGS += -march=armv4t
	CFLAGS += $(EXCFLAGS) -DEMSYS_AT91RM9200
endif

#
LIB_OBJS = libs_else.o

# Add usart
ifeq ($(CFG_USART_LIB),y)
  LIB_OBJS += libs_uart.o
  CFLAGS += -DCFG_USART_LIB
endif
#
ifeq ($(CFG_ADC_LIB),y)
  LIB_OBJS += libs_adc.o
  CFLAGS += -DCFG_ADC_LIB
endif
#
ifeq ($(CFG_DAC_LIB),y)
  LIB_OBJS += libs_dac.o
  CFLAGS += -DCFG_DAC_LIB
endif
#
ifeq ($(CFG_GPIO_LIB),y)
  LIB_OBJS += libs_io.o
  CFLAGS += -DCFG_GPIO_LIB
endif
#
ifeq ($(CFG_PORT_LIB),y)
  LIB_OBJS += libs_port.o
  CFLAGS += -DCFG_PORT_LIB
endif
ifeq ($(CFG_DATETIM_LIB),y)
  LIB_OBJS += libs_time.o
  CFLAGS += -DCFG_DATETIM_LIB
endif
#
ifeq ($(CFG_TIMER_LIB),y)
  LIB_OBJS += libs_timer.o
  CFLAGS += -DCFG_TIMER_LIB
endif
#
ifeq ($(CFG_HIGHTIMER_LIB),y)
  LIB_OBJS += libs_hightimer.o
  CFLAGS += -DCFG_HIGHTIMER_LIB
  EXTERLIBS += -lrt
endif
#
ifeq ($(CFG_SERIALID_LIB),y)
  LIB_OBJS += libs_serialid.o
  CFLAGS += -DCFG_SERIALID_LIB
endif
#
ifeq ($(CFG_TEMPER_LIB),y)
  LIB_OBJS += libs_temper.o
  CFLAGS += -DCFG_TEMPER_LIB
endif
#
ifeq ($(CFG_PWM_LIB),y)
  LIB_OBJS += libs_pwmout.o
  CFLAGS += -DCFG_PWM_LIB
endif
#
ifeq ($(CFG_PULSEI_LIB),y)
  LIB_OBJS += libs_picap.o
  CFLAGS += -DCFG_PULSEI_LIB
endif
#
ifeq ($(CFG_WDT_LIB),y)
  LIB_OBJS += libs_wdt.o
  CFLAGS += -DCFG_WDT_LIB
endif
#
ifeq ($(CFG_KBD_LIB),y)
  LIB_OBJS += libs_key.o
  CFLAGS += -DCFG_KBD_LIB
endif

#指定编译的独立文件
obj_libs = $(LIB_OBJS)

all:lib$(LIBSNAME).so.1.0.0 lib$(LIBSNAME).so lib$(LIBSNAME).a

# 建立符号连接
lib$(LIBSNAME).so: lib$(LIBSNAME).so.1.0.0
#	ln -sf $< $@
	cp -af $< $@

# 由指定的多个目标文件生成共享库
lib$(LIBSNAME).so.1.0.0:$(obj_libs)
	$(CC) $(EXCFLAGS) -g -shared -Wl,-soname,lib$(LIBSNAME).so -o $@ $^ -lc $(EXTERLIBS)
	$(STRIP) $@

# 由指定的多个目标文件生成静态库
lib$(LIBSNAME).a:$(obj_libs)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf *.so* *.o *.a *.O
