CC = /opt/arm-2007q3/bin/arm-none-linux-gnueabi-gcc
CPPFLAGS = -Wall -g -Os -lpthread -lstdc++ -fomit-frame-pointer
PREDEFINES = -DLINUX -D__OS__=LINUX -DNDEBUG
CFLAGS = $(CPPFLAGS) $(PREDEFINES)
RM = rm
LDFLAGS =  -L../lib/libs_emsys_odm -l EMZY_ARM_ODM
INCLUDE = -I.


TARGET = concentrator

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

%.o:%.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

SOURCES = $(wildcard *.c *.cpp)
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -rf *.o $(TARGET)
