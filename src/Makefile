include ../script/makefile.include
program_NAME := concentrator
program_SRCS := $(wildcard *.c *.cpp)
program_OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(program_SRCS)))#${program_SRCS:.c=.o .cpp=.o}
clean_list += $(program_OBJS) $(program_NAME)

RM = rm
CC = $(BASE)arm-none-linux-gnueabi-gcc
STRIP = $(BASE)arm-none-linux-gnueabi-strip
CPPFLAGS += -Wall -g -O2 -lpthread -lstdc++ -fomit-frame-pointer
PREDEFINES = -DLINUX -D__OS__=LINUX -DNDEBUG -DDEBUG_CARDHOST -DDEBUG_MAIN -DDEBUG_FORWARDER
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
	@- $(RM) -f $(clean_list)

# Generate dependencies for all files in project
%.d: %.c
	@ $(CC) $(CPPFLAGS) -MM $< | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' | sed -e 's/\.o: /\.o: Makefile /g' > $@
%.d: %.cpp
	@ $(CC) $(CPPFLAGS) -MM $< | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' | sed -e 's/\.o: /\.o: Makefile /g' > $@

clean_list += ${program_OBJS:.o=.d}

$(program_NAME): $(program_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(program_OBJS) -o $(program_NAME)
	$(STRIP) $(program_NAME)

ifneq "$(MAKECMDGOALS)" "clean"
# Include the list of dependancies generated for each object file
-include ${program_OBJS:.o=.d}
endif
