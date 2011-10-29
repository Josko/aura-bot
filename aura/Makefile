SHELL = /bin/sh
SYSTEM = $(shell uname)
ARCH = $(shell uname -m)
CC = gcc
CXX = g++
CCFLAGS = -fno-builtin
CXXFLAGS = -pipe -Wall -Wextra -fno-builtin
DFLAGS = 
OFLAGS = -O3
LFLAGS = -L. -L../StormLib/stormlib/ -L../bncsutil/src/bncsutil/ -lStorm -lbncsutil -lpthread -ldl -lz -lboost_date_time -lboost_system -lboost_filesystem

ifeq ($(ARCH),x86_64)
	CCFLAGS += -m64
	CXXFLAGS += -m64
endif

ifeq ($(SYSTEM),Darwin)
	CC = /usr/bin/gcc
	CXX = /usr/bin/g++
	DFLAGS += -D__APPLE__
	OFLAGS += -flat_namespace
	LFLAGS += -L/opt/local/lib/
else
	LFLAGS += -lrt
endif

ifeq ($(SYSTEM),FreeBSD)
	DFLAGS += -D__FREEBSD__
endif

ifeq ($(SYSTEM),SunOS)
	DFLAGS += -D__SOLARIS__
	LFLAGS += -lresolv -lsocket -lnsl
endif

CCFLAGS += $(OFLAGS) -DSQLITE_THREADSAFE=0 -I.
CXXFLAGS += $(OFLAGS) $(DFLAGS) -I. -I../bncsutil/src/ -I../StormLib/

OBJS = bncsutilinterface.o bnet.o bnetprotocol.o config.o crc32.o csvparser.o game.o gameplayer.o gameprotocol.o gameslot.o gpsprotocol.o aura.o auradb.o map.o sha1.o socket.o stats.o irc.o
COBJS = sqlite3.o
PROGS = aura++

all: $(OBJS) $(COBJS) $(PROGS)
	@echo "Used CFLAGS: $(CXXFLAGS)"

aura++: $(OBJS) $(COBJS)
	@$(CXX) -o aura++ $(OBJS) $(COBJS) $(LFLAGS)
	@echo "[BIN] Success. $@ created."

clean:
	@rm -f $(OBJS) $(COBJS) $(PROGS)
	@echo "Binary and object files cleaned."

install:
	@cp $(PROGS) ..

$(OBJS): %.o: %.cpp
	@$(CXX) -o $@ $(CXXFLAGS) -c $<
	@echo "[$(CXX)] $@"

$(COBJS): %.o: %.c
	@$(CC) -o $@ $(CCFLAGS) -c $<
	@echo "[$(CC)] $@"

