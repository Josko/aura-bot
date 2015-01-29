SHELL = /bin/sh
SYSTEM = $(shell uname)
ARCH = $(shell uname -m)

ifndef CC
  CC = gcc
endif

ifndef CXX
  CXX = g++
endif

CCFLAGS = -fno-builtin
CXXFLAGS = -std=c++11 -pipe -Wall -Wextra -fno-builtin
DFLAGS = 
OFLAGS = -O3
LFLAGS = -L. -L/usr/local/lib/ -L../bncsutil/src/bncsutil/ -lstorm -lbncsutil -ldl -lgmp

ifeq ($(ARCH),x86_64)
	CCFLAGS += -m64
	CXXFLAGS += -m64
endif

ifeq ($(SYSTEM),Darwin)
	CXXFLAGS += -stdlib=libc++
	CC = clang
	CXX = clang++
	OFLAGS = -O4
	DFLAGS += -D__APPLE__
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
CXXFLAGS += $(OFLAGS) $(DFLAGS) -I. -I../bncsutil/src/ -I../StormLib/src/

OBJS = aura/bncsutilinterface.o aura/bnet.o aura/bnetprotocol.o aura/config.o aura/crc32.o aura/csvparser.o aura/game.o aura/gameplayer.o aura/gameprotocol.o aura/gameslot.o aura/gpsprotocol.o aura/aura.o aura/auradb.o aura/map.o aura/sha1.o aura/socket.o aura/stats.o aura/irc.o aura/fileutil.o
COBJS = aura/sqlite3.o
PROGS = aura++

all: $(OBJS) $(COBJS) $(PROGS)
	@echo "Used CFLAGS: $(CXXFLAGS)"

aura++: $(OBJS) $(COBJS)
	@$(CXX) -o aura++ $(OBJS) $(COBJS) $(CXXFLAGS) $(LFLAGS)
	@echo "[BIN] $@ created."
	@strip aura++
	@echo "[BIN] Stripping the binary."

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
