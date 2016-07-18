SUBDIRS=src
CLEANSUBDIRS=test examples

all: subdirs

test:
	@${MAKE} -C test 

examples:
	@${MAKE} -C examples

coverage:
	@(cd src && ( find . -name "*.c" | xargs gcov -p ) )

ROOTDIR=.
include $(ROOTDIR)/rules.make
