SUBDIRS=src
CLEANSUBDIRS=test examples

all: subdirs

test:
	@${MAKE} -C test 

examples:
	@${MAKE} -C examples

coverage:
	@for cfile in $$(find ./src ./test ./examples -name "*.c"); \
	do \
		( cd $$(dirname $${cfile}) && gcov $$(basename $${cfile}) ) ; \
	done

ROOTDIR=.
include $(ROOTDIR)/rules.make
