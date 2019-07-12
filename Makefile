ROOTDIR=.
include $(ROOTDIR)/config.make

SUBDIRS=src
CLEANSUBDIRS=test examples

BIN=wintest.exe
OBJS=wintest.o
LDLIBS+=-L$(ROOTDIR)/src -lft

all: subdirs ${BIN}

test:
	@${MAKE} -C test 

examples:
	@${MAKE} -C examples

coverage:
	@${MAKE} -C src coverage


include $(ROOTDIR)/rules.make
