#####################################################################
###
#
# Makefile for compiling StormLib under linux
#
# Author: Marko Friedemann <marko.friedemann@bmx-chemnitz.de>
# Created at: Mon Jan 29 18:26:01 CEST 2001
# Computer: whiplash.flachland-chemnitz.de 
# System: Linux 2.4.0 on i686
#    
# Copyright (c) 2001 BMX-Chemnitz.DE All rights reserved.
#
#####################################################################
###

##############################################
# updated on October 6, 2008 by Trevor Hogan #
##############################################

#################################################
# updated on January 6, 2010 by Julian "Argon-" #
#################################################

SHELL = /bin/sh
SYSTEM = $(shell uname)
CXX = g++
CC = gcc
DFLAGS = -D__SYS_ZLIB
OFLAGS = -O2
LFLAGS = -lbz2 -lz
CFLAGS = -fPIC
CFLAGS += $(OFLAGS) $(DFLAGS)

OBJS = SCommon.o SCompression.o SFileCompactArchive.o \
       SFileCreateArchiveEx.o SFileExtractFile.o SFileFindFile.o \
       SListFile.o SFileOpenArchive.o SFileOpenFileEx.o \
       SAttrFile.o SFileReadFile.o \
       wave/wave.o \
       huffman/huff.o \
       misc/crc32.o misc/md5.o

COBJS = pklib/crc32.o pklib/explode.o pklib/implode.o

ifeq ($(SYSTEM),Darwin)
	CXX = /usr/bin/g++
	CC = /usr/bin/gcc
	OBJS += StormPortMac.o
	LFLAGS += -dynamiclib -framework CoreServices
	LIB = libStorm.dylib
else
	OBJS += StormPortLinux.o
	LFLAGS += -shared
	LIB = libStorm.so
endif

all: $(OBJS) $(COBJS) $(LIB)

$(LIB): $(OBJS) $(COBJS)
	$(CXX) -o $(LIB) $(OBJS) $(COBJS) $(LFLAGS)

clean:
	rm -f $(OBJS) $(COBJS) $(LIB)

$(OBJS): %.o: %.cpp
	$(CXX) -o $@ $(CFLAGS) -c $<

$(COBJS): %.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

$(LIB): $(OBJS) $(COBJS)

all: $(LIB)

install: $(LIB)
	install $(LIB) /usr/lib
	mkdir -p /usr/include/StormLib
	cp StormLib.h /usr/include/StormLib
	cp StormPort.h /usr/include/StormLib
	ldconfig
