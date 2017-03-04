OBJS=
ifdef RECV
OBJS+=mp-recv.o
endif
ifdef SEND
OBJS+=mp-send.o
endif

EXTRACLEAN=mp-recv mp-send mp-recv.o mp-send.o

LDLIBS+=-L$(ROOTDIR)/src -lft /opt/msgpack-c/lib/libmsgpackc.a

all:
	@BIN=pb-send SEND=1 ${MAKE} mp-send
	@BIN=pb-recv RECV=1 ${MAKE} mp-recv

ROOTDIR=../..
include $(ROOTDIR)/rules.make

