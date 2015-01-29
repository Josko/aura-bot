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
OFLAGS = -O3 -flto
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

OBJS = src/bncsutilinterface.o \
			 src/bnet.o \
			 src/bnetprotocol.o \
			 src/config.o \
			 src/crc32.o \
			 src/csvparser.o \
			 src/game.o \
			 src/gameplayer.o \
			 src/gameprotocol.o \
			 src/gameslot.o \
			 src/gpsprotocol.o \
			 src/aura.o \
			 src/auradb.o \
			 src/map.o \
			 src/sha1.o \
			 src/socket.o \
			 src/stats.o \
			 src/irc.o \
			 src/fileutil.o

COBJS = src/sqlite3.o

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
	@install -D $(PROGS) "$(DESTDIR)/usr/bin/$(PROGS)"
	@echo "Binary $(PROGS) installed to $(DESTDIR)/usr/bin"

$(OBJS): %.o: %.cpp
	@$(CXX) -o $@ $(CXXFLAGS) -c $<
	@echo "[$(CXX)] $@"

$(COBJS): %.o: %.c
	@$(CC) -o $@ $(CCFLAGS) -c $<
	@echo "[$(CC)] $@"
