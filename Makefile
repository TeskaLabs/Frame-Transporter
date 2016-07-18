SUBDIRS=src
CLEANSUBDIRS=test examples

all: subdirs

test:
	@${MAKE} -C test 

examples:
	@${MAKE} -C examples

coverage:
	@(cd ./examples/forward && ( find . -name "*.c" | xargs gcov ) )
	@(cd ./examples/reply && ( find . -name "*.c" | xargs gcov ) )
	@(cd ./examples/sfetch && ( find . -name "*.c" | xargs gcov ) )
	@(cd ./examples/sreply && ( find . -name "*.c" | xargs gcov ) )
	@(cd ./examples/stun && ( find . -name "*.c" | xargs gcov ) )
	@(cd ./test && ( find . -name "*.c" | xargs gcov ) )
	@find ./examples ./test -name "*.gcov" | xargs rm
	@(cd ./src && ( find . -name "*.c" | xargs gcov ) )

ROOTDIR=.
include $(ROOTDIR)/rules.make
