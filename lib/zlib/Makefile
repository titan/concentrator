include ../../script/makefile.include
CC=$(BASE)arm-none-linux-gnueabi-gcc
CFLAGS=-O3 -fPIC -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN -DPIC
STRIP=$(BASE)arm-none-linux-gnueabi-strip
AR=$(BASE)arm-none-linux-gnueabi-ar
EXCFLAGS:=
EXTERLIBS:=
LIBSNAME=z

LIB_OBJS = adler32.o crc32.o deflate.o infback.o inffast.o inflate.o inftrees.o trees.o zutil.o compress.o uncompr.o gzclose.o gzlib.o gzread.o gzwrite.o

obj_libs = $(LIB_OBJS)

all:lib$(LIBSNAME).a

lib$(LIBSNAME).a:$(obj_libs)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf *.o *.a
