program_NAME := test-logdb test-logqueue

test-logdb_SRCS := test-logdb.c ../src/logdb.c ../src/sbtree.c
test-logdb_OBJS := $(patsubst %.c,%.o,$(test-logdb_SRCS))

test-logqueue_SRCS := test-logqueue.c ../src/logqueue.c
test-logqueue_OBJS := $(patsubst %.c,%.o,$(test-logqueue_SRCS))

clean_list += $(test-logdb_OBJS) $(test-logqueue_OBJS) $(program_NAME)

RM = rm
CC = gcc
STRIP = strip
CFLAGS += -Wextra -O2 -g
LDFLAGS =
INCLUDE = -I../src/ -I.

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: all clean

all: $(program_NAME)

clean:
	@- $(RM) -f $(clean_list)

# Generate dependencies for all files in project
%.d: %.c
	@ $(CC) $(CFLAGS) $(INCLUDE) -MM $< | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' | sed -e 's/\.o: /\.o: Makefile /g' > $@

clean_list += ${test-logdb_OBJS:.o=.d} ${test-logqueue_OBJS:.o=.d}

test-logdb: $(test-logdb_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(test-logdb_OBJS) -o $@

test-logqueue: $(test-logqueue_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(test-logqueue_OBJS) -o $@

ifneq "$(MAKECMDGOALS)" "clean"
# Include the list of dependancies generated for each object file
-include ${test-logdb_OBJS:.o=.d}
-include ${test-logqueue_OBJS:.o=.d}
endif
