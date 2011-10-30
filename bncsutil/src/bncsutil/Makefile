SHELL = /bin/sh
SYSTEM = $(shell uname)
ARCH = $(shell uname -m)
CC = gcc
CXX = g++
CCFLAGS = -O3 -I ../ -Wno-multichar -fPIC
CXXFLAGS = -O3 -I ../ -Wno-multichar -fPIC
CCOBJ = nls.o pe.o sha1.o stack.o
CXXOBJ = bsha1.o cdkeydecoder.o checkrevision.o decodekey.o file.o libinfo.o oldauth.o

ifeq ($(ARCH),x86_64)
	CCFLAGS += -m64
	CXXFLAGS += -m64
endif

ifeq ($(SYSTEM),Darwin)
	CXX = /usr/bin/g++
	CC = /usr/bin/gcc
	LDFLAGS = -dynamiclib -lgmp -L/opt/local/lib
	TARGET = libbncsutil.a
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

# vim: set noexpandtab ts=4 sw=4:
