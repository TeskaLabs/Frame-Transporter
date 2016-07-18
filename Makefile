SUBDIRS=src
CLEANSUBDIRS=test examples

all: subdirs

test:
	@${MAKE} -C test 

examples:
	@${MAKE} -C examples

coverage:
	@(cd test && gcov -p *.c)
	@(cd src && ( find . -name "*.c" | xargs gcov -p ) )
	@(cd examples/forward && gcov -p *.c)
	@(cd examples/reply && gcov -p *.c)
	@(cd examples/sfetch && gcov -p *.c)
	@(cd examples/sreply && gcov -p *.c)
	@(cd examples/stun && gcov -p *.c)

ROOTDIR=.
include $(ROOTDIR)/rules.make
