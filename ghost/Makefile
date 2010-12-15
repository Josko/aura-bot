SHELL = /bin/sh
SYSTEM = $(shell uname)
C++ = g++
CC = gcc
DFLAGS = -Wall -DSQLITE_THREADSAFE=0 -m64
OFLAGS = -O3
LFLAGS = -L. -L../bncsutil/src/bncsutil/ -L../StormLib/stormlib/ -lbncsutil -lpthread -ldl -lz -lStorm -lboost_date_time-mt -lboost_system-mt -lboost_filesystem-mt
CFLAGS =

ifeq ($(SYSTEM),Darwin)
DFLAGS += -D__APPLE__
OFLAGS += -flat_namespace
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

CFLAGS += $(OFLAGS) $(DFLAGS) -I. -I../bncsutil/src/ -I../StormLib/

OBJS = bncsutilinterface.o bnet.o bnetprotocol.o commandpacket.o config.o crc32.o csvparser.o game.o gameplayer.o gameprotocol.o gameslot.o ghost.o ghostdb.o ghostdbsqlite.o gpsprotocol.o language.o map.o sha1.o socket.o stats.o statsdota.o util.o irc.o
COBJS = sqlite3.o
PROGS = ./ghost++

all: $(OBJS) $(COBJS) $(PROGS)

./ghost++: $(OBJS) $(COBJS)
	$(C++) -o ./ghost++ $(OBJS) $(COBJS) $(LFLAGS)

clean:
	rm -f $(OBJS) $(COBJS) $(PROGS)

$(OBJS): %.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -c $<

$(COBJS): %.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

./ghost++: $(OBJS) $(COBJS)

all: $(PROGS)

bncsutilinterface.o: ghost.h includes.h util.h bncsutilinterface.h
bnet.o: ghost.h includes.h util.h config.h language.h socket.h commandpacket.h ghostdb.h bncsutilinterface.h bnetprotocol.h bnet.h map.h gameprotocol.h
bnetprotocol.o: ghost.h includes.h util.h bnetprotocol.h
commandpacket.o: ghost.h includes.h commandpacket.h
config.o: ghost.h includes.h config.h
crc32.o: ghost.h includes.h crc32.h
csvparser.o: csvparser.h
game.o: ghost.h includes.h util.h config.h language.h socket.h ghostdb.h bnet.h map.h gameplayer.h gameprotocol.h game.h stats.h statsdota.h
gameplayer.o: ghost.h includes.h util.h language.h socket.h commandpacket.h bnet.h map.h gameplayer.h gameprotocol.h gpsprotocol.h
gameprotocol.o: ghost.h includes.h util.h crc32.h gameplayer.h gameprotocol.h
gameslot.o: ghost.h includes.h gameslot.h
ghost.o: ghost.h includes.h util.h crc32.h sha1.h csvparser.h config.h language.h socket.h ghostdb.h ghostdbsqlite.h bnet.h map.h gameplayer.h gameprotocol.h gpsprotocol.h game.h
ghostdb.o: ghost.h includes.h util.h config.h ghostdb.h
ghostdbsqlite.o: ghost.h includes.h util.h config.h ghostdb.h ghostdbsqlite.h
gpsprotocol.o: ghost.h util.h gpsprotocol.h
language.o: ghost.h includes.h config.h language.h
map.o: ghost.h includes.h util.h crc32.h sha1.h config.h map.h
sha1.o: sha1.h
socket.o: ghost.h includes.h util.h socket.h
stats.o: ghost.h includes.h stats.h
statsdota.o: ghost.h includes.h util.h ghostdb.h gameplayer.h gameprotocol.h stats.h statsdota.h
util.o: ghost.h includes.h util.h
irc.o: ghost.h includes.h irc.h socket.h util.h bnetprotocol.h bnet.h
