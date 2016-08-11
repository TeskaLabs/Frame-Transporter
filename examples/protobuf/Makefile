OBJS=protobuf-c/protobuf-c.o amessage.pb-c.o
ifdef RECV
OBJS+=pb-recv.o
endif
ifdef SEND
OBJS+=pb-send.o
endif

EXTRACLEAN=amessage.pb-c.c amessage.pb-c.h pb-recv pb-send pb-recv.o pb-send.o

LDLIBS+=-L$(ROOTDIR)/src -lft

all: amessage.pb-c.c
	@BIN=pb-send SEND=1 ${MAKE} pb-send
	@BIN=pb-recv RECV=1 ${MAKE} pb-recv

pb-recv.o: amessage.pb-c.h

pb-send.o: amessage.pb-c.h

%.pb-c.h %.pb-c.c: %.proto
	@echo " [PC]" $@
	@protoc-c --c_out=. $<

ROOTDIR=../..
include $(ROOTDIR)/rules.make

