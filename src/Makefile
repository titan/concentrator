program_NAME := concentrator
program_SRCS := $(wildcard *.c *.cpp)
program_OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(program_SRCS)))#${program_SRCS:.c=.o .cpp=.o}
clean_list += $(program_OBJS) $(program_NAME)

RM = rm
CC = /opt/arm-2007q3/bin/arm-none-linux-gnueabi-gcc
CPPFLAGS += -Wall -g -O2 -lpthread -lstdc++ -fomit-frame-pointer
PREDEFINES = -DLINUX -D__OS__=LINUX -DNDEBUG
CFLAGS += -Wextra $(CPPFLAGS) $(PREDEFINES)
LDFLAGS =  -L../lib/libs_emsys_odm -l EMZY_ARM_ODM
INCLUDE = -I.

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: all clean

all: $(program_NAME)

clean:
	@- $(RM) $(clean_list)

# Generate dependencies for all files in project
%.d: $(program_SRCS)
	@ $(CC) $(CPPFLAGS) -MM $< | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@

clean_list += ${program_OBJS:.o=.d}

$(program_NAME): $(program_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(program_OBJS) -o $(program_NAME)

ifneq "$(MAKECMDGOALS)" "clean"
# Include the list of dependancies generated for each object file
-include ${program_OBJS:.o=.d}
endif
