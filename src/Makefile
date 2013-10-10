include ../script/makefile.include
program_NAME := concentrator
program_SRCS := $(wildcard *.c *.cpp)
program_OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(program_SRCS)))
progens_SRCS := $(patsubst %.cpp.orig, %.cpp, $(wildcard *.cpp.orig))
progens_OBJS := $(patsubst %.cpp,%.o,$(progens_SRCS))
protobuf_GENS := $(patsubst %.proto,%.pb.cc,$(wildcard *.proto)) $(patsubst %.proto,%.pb.h,$(wildcard *.proto))
protobuf_OBJS := $(patsubst %.proto,%.pb.o,$(wildcard *.proto))
clean_list += $(program_OBJS) $(progens_OBJS) $(program_NAME) $(progens_SRCS) $(protobuf_GENS) $(protobuf_OBJS)

.INTERMEDIATE:$(progens_SRCS)

RM = rm
CC = $(BASE)arm-none-linux-gnueabi-gcc
STRIP = $(BASE)arm-none-linux-gnueabi-strip
CPPFLAGS += -Wall -g -O2 -lpthread -lstdc++ -fomit-frame-pointer
PREDEFINES = -DLINUX -D__OS__=LINUX -DNDEBUG -DDEBUG_MAIN -DDEBUG_MAINLOOP -DDEBUG_FORWARDER -DDEBUG_VALVE -DDEBUG_CARDHOST # -DDEBUG_HEAT -DDEBUG_VALVE_TRACE_RECHARGE -DDEBUG_PORTAL -DDEBUG_GPRS
CFLAGS += -Wextra $(CPPFLAGS) $(PREDEFINES)
LDFLAGS =  -L../lib/libs_emsys_odm -l EMZY_ARM_ODM -L../lib/protobuf -l protobuf-lite
INCLUDE = -I. -I../lib/protobuf -I../lib/zlib/
TRANS-HZ = ../util/trans-hz.py
PROTOC = protoc

%.pb.cc: %.proto
	$(PROTOC) --cpp_out=. $<

%.cpp: %.cpp.orig
	$(TRANS-HZ) $< $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.cc
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: all clean

all: $(program_NAME)

clean:
	@- $(RM) -f $(clean_list)

# Generate dependencies for all files in project
%.d: %.c
	@ $(CC) $(CFLAGS) $(INCLUDE) -MM $< | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' | sed -e 's/\.o: /\.o: Makefile /g' > $@
%.d: %.cpp
	@ $(CC) $(CFLAGS) $(INCLUDE) -MM $< | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' | sed -e 's/\.o: /\.o: Makefile /g' > $@

clean_list += ${program_OBJS:.o=.d}

$(program_NAME): $(protobuf_OBJS) $(program_OBJS) $(progens_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(program_OBJS) $(progens_OBJS) $(protobuf_OBJS) ../lib/zlib/libz.a -o $(program_NAME)
	$(STRIP) $(program_NAME)

ifneq "$(MAKECMDGOALS)" "clean"
# Include the list of dependancies generated for each object file
-include ${program_OBJS:.o=.d}
endif
