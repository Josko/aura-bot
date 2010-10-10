SHELL = /bin/sh
SYSTEM = $(shell uname)
CXX = g++
CXXFLAGS = -Wall -O3 -I ../ -Wno-multichar -fPIC
CXXOBJ = bsha1.o cdkeydecoder.o checkrevision.o decodekey.o file.o libinfo.o oldauth.o
CC = gcc
CCFLAGS = -Wall -O3 -I ../ -Wno-multichar -fPIC
CCOBJ = nls.o pe.o sha1.o stack.o

ifeq ($(SYSTEM),Darwin)
LDFLAGS = -dynamiclib -lgmp -L/opt/local/lib
TARGET = libbncsutil.dylib
else
LDFLAGS = -shared -lgmp
TARGET = libbncsutil.so
endif

$(TARGET): $(CXXOBJ) $(CCOBJ)
	$(CXX)  $(CXXFLAGS) $(LDFLAGS) $(CXXOBJ) $(CCOBJ) -o $(TARGET)

$(CXXOBJ): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ 

$(CCOBJ): %.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(CCOBJ) $(CXXOBJ) $(TARGET) *~
all:
	make $(TARGET)

install: $(TARGET)
	mkdir -p /usr/include/bncsutil
	cp *.h /usr/include/bncsutil
	cp $(TARGET) /usr/lib
	ldconfig
