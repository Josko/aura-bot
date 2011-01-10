SHELL = /bin/sh
SYSTEM = $(shell uname)
C++ = g++
CC = gcc
DFLAGS = -Wall -DSQLITE_THREADSAFE=0 -m64
OFLAGS = -O3
LFLAGS = -L. -L../StormLib/stormlib/ -L../bncsutil/src/bncsutil/ -lStorm -lbncsutil -lpthread -ldl -lz -lboost_date_time-mt -lboost_system-mt -lboost_filesystem-mt
CFLAGS = -Wall -m64

ifeq ($(SYSTEM),Darwin)
C++ = /usr/bin/g++
CC = /usr/bin/gcc
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

CFLAGS += $(OFLAGS) $(DFLAGS) -I. -I../bncsutil/src/ -I../StormLib/

OBJS = bncsutilinterface.o bnet.o bnetprotocol.o config.o crc32.o csvparser.o game.o gameplayer.o gameprotocol.o gameslot.o aura.o auradb.o gpsprotocol.o language.o map.o sha1.o socket.o stats.o util.o irc.o
COBJS = sqlite3.o
PROGS = ./aura++

all: $(OBJS) $(COBJS) $(PROGS)

./aura++: $(OBJS) $(COBJS)
	$(C++) -o ./aura++ $(OBJS) $(COBJS) $(LFLAGS)

clean:
	rm -f $(OBJS) $(COBJS) $(PROGS)	

install: $(PROGS)
	mv $(PROGS) ../$(PROGS)

$(OBJS): %.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -c $<

$(COBJS): %.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

./aura++: $(OBJS) $(COBJS)

all: $(PROGS)

bncsutilinterface.o: aura.h includes.h util.h bncsutilinterface.h
bnet.o: aura.h includes.h util.h config.h language.h socket.h auradb.h bncsutilinterface.h bnetprotocol.h bnet.h map.h gameprotocol.h
bnetprotocol.o: aura.h includes.h util.h bnetprotocol.h
config.o: aura.h includes.h config.h
crc32.o: aura.h includes.h crc32.h
csvparser.o: csvparser.h
game.o: aura.h includes.h util.h config.h language.h socket.h auradb.h bnet.h map.h gameplayer.h gameprotocol.h game.h stats.h
gameplayer.o: aura.h includes.h util.h language.h socket.h bnet.h map.h gameplayer.h gameprotocol.h gpsprotocol.h
gameprotocol.o: aura.h includes.h util.h crc32.h gameplayer.h gameprotocol.h
gameslot.o: aura.h includes.h gameslot.h
aura.o: aura.h includes.h util.h crc32.h sha1.h csvparser.h config.h language.h socket.h auradb.h bnet.h map.h gameplayer.h gameprotocol.h gpsprotocol.h game.h
auradb.o: aura.h includes.h util.h config.h auradb.h
gpsprotocol.o: aura.h util.h gpsprotocol.h
language.o: aura.h includes.h config.h language.h
map.o: aura.h includes.h util.h crc32.h sha1.h config.h map.h
sha1.o: sha1.h
socket.o: aura.h includes.h util.h socket.h
stats.o: aura.h includes.h stats.h
util.o: aura.h includes.h util.h
irc.o: aura.h includes.h irc.h socket.h util.h bnetprotocol.h bnet.h
