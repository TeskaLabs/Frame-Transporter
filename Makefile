SUBDIRS=src
CLEANSUBDIRS=test examples

all: subdirs

test:
	@${MAKE} -C test 

examples:
	@${MAKE} -C examples

coverage:
	@(cd test && gcov -p *.c)
	@(cd src && gcov -p *.c)

ROOTDIR=.
include $(ROOTDIR)/rules.make
