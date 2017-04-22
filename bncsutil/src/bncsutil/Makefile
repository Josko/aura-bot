SHELL = /bin/sh
SYSTEM = $(shell uname)
CXX = g++
CXXFLAGS = -O3 -I ../ -Wno-multichar -fPIC -flto
CXXOBJ = bsha1.o cdkeydecoder.o checkrevision.o decodekey.o file.o libinfo.o oldauth.o
CC = gcc
CCFLAGS = -O3 -I ../ -Wno-multichar -fPIC
CCOBJ = nls.o pe.o sha1.o stack.o

ifeq ($(SYSTEM),Darwin)
CXX = /usr/bin/g++
CC = /usr/bin/gcc
LDFLAGS = -dynamiclib -lgmp -L/opt/local/lib -L/usr/local/lib
TARGET = libbncsutil.a
CCFLAGS += -m64
CXXFLAGS += -m64
else
LDFLAGS = -shared -lgmp
TARGET = libbncsutil.so
endif

$(TARGET): $(CXXOBJ) $(CCOBJ)
	$(CXX)  $(CXXFLAGS) $(LDFLAGS) $(CXXOBJ) $(CCOBJ) -flto -o $(TARGET)
ifeq ($(SYSTEM),Darwin)
	@strip -x $(TARGET)
else
	@strip $(TARGET)
endif

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
