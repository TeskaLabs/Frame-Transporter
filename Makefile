SUBDIRS=src
CLEANSUBDIRS=test examples

all: subdirs

test:
	@${MAKE} -C test 

examples:
	@${MAKE} -C examples

ROOTDIR=.
include $(ROOTDIR)/rules.make
