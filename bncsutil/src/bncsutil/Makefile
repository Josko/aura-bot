SHELL = /bin/sh
SYSTEM = $(shell uname)
INSTALL_DIR = /usr
CXX = g++
CXXFLAGS = -std=c++11 -O3 -I ../ -Wno-multichar -fPIC -flto
CXXOBJ = bsha1.o cdkeydecoder.o checkrevision.o decodekey.o file.o libinfo.o oldauth.o
CC = gcc
CCFLAGS = -std=c99 -O3 -I ../ -Wno-multichar -fPIC
CCOBJ = nls.o pe.o sha1.o stack.o

ifeq ($(SYSTEM),Darwin)
	INSTALL_DIR = /usr/local
	CXX = clang++
	CC = clang
	LDFLAGS = -dynamiclib -lgmp -L/usr/local/lib
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
	@install -d "$(DESTDIR)$(INSTALL_DIR)/include/bncsutil"
	install *.h "$(DESTDIR)$(INSTALL_DIR)/include/bncsutil/"
	install $(TARGET) "$(DESTDIR)$(INSTALL_DIR)/lib/"
ifneq ($(SYSTEM),Darwin)
	ldconfig
endif
