SHELL = /bin/sh
SYSTEM = $(shell uname)
CC = gcc
CXX = g++
CCFLAGS = -m64
CXXFLAGS = -m64 -Wall -Wextra
DFLAGS = 
OFLAGS = -O2
LFLAGS = -L. -L../StormLib/stormlib/ -L../bncsutil/src/bncsutil/ -lStorm -lbncsutil -lpthread -ldl -lz -lboost_date_time-mt -lboost_system-mt -lboost_filesystem-mt

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

OBJS = bncsutilinterface.o bnet.o bnetprotocol.o config.o crc32.o csvparser.o game.o gameplayer.o gameprotocol.o gameslot.o aura.o auradb.o gpsprotocol.o language.o map.o sha1.o socket.o stats.o util.o irc.o
COBJS = sqlite3.o
PROGS = ./aura++

all: $(OBJS) $(COBJS) $(PROGS)

./aura++: $(OBJS) $(COBJS)
	$(CXX) -o ./aura++ $(OBJS) $(COBJS) $(LFLAGS)

clean:
	rm -f $(OBJS) $(COBJS) $(PROGS)	

install: $(PROGS)
	cp $(PROGS) ../$(PROGS)

$(OBJS): %.o: %.cpp
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(COBJS): %.o: %.c
	$(CC) -o $@ $(CCFLAGS) -c $<

./aura++: $(OBJS) $(COBJS)
