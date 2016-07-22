SUBDIRS=src
CLEANSUBDIRS=test examples

all: subdirs

test:
	@${MAKE} -C test 

examples:
	@${MAKE} -C examples

coverage:
	@${MAKE} -C src coverage

ROOTDIR=.
include $(ROOTDIR)/rules.make
