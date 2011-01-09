SHELL = /bin/sh
SYSTEM = $(shell uname)
CXX = g++
CXXFLAGS = -I ../ -Wno-multichar -fPIC
CXXOBJ = bsha1.o cdkeydecoder.o checkrevision.o decodekey.o file.o libinfo.o oldauth.o
CC = gcc
CCFLAGS = -I ../ -Wno-multichar -fPIC
CCOBJ = nls.o pe.o sha1.o stack.o

ifeq ($(SYSTEM),Darwin)
CXX = /usr/bin/g++
CC = /usr/bin/gcc
LDFLAGS = -dynamiclib -lgmp -L/opt/local/lib
TARGET = libbncsutil.a
CCFLAGS += -m64
CXXFLAGS += -m64
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
	mkdir -p /usr/local/include/bncsutil
	cp *.h /usr/local/include/bncsutil
	cp $(TARGET) /usr/local/lib
	ldconfig
