include ../../script/makefile.include
program_NAME := test-read test-write echo-client echo-server
program_SRCS := $(wildcard *.c)
program_OBJS := $(patsubst %.c,%.o,$(program_SRCS))
clean_list += $(program_OBJS) $(program_NAME)

RM = rm
CC = $(BASE)arm-none-linux-gnueabi-gcc
STRIP = $(BASE)arm-none-linux-gnueabi-strip
CFLAGS += -Wextra -O2 -g
LDFLAGS =  -L../../lib/libs_emsys_odm/ -l EMZY_ARM_ODM
INCLUDE = -I../../lib/libs_emsys_odm/

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: all clean

all: $(program_NAME)

clean:
	@- $(RM) -f $(clean_list)

# Generate dependencies for all files in project
%.d: %.c
	@ $(CC) $(CFLAGS) -MM $< | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' | sed -e 's/\.o: /\.o: Makefile /g' > $@

clean_list += ${program_OBJS:.o=.d}

$(program_NAME): $(program_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o -o $@

ifneq "$(MAKECMDGOALS)" "clean"
# Include the list of dependancies generated for each object file
-include ${program_OBJS:.o=.d}
endif
