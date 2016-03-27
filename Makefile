SUBDIRS=src
CLEANSUBDIRS=test

all: subdirs

test:
	@${MAKE} -C test 

ROOTDIR=.
include $(ROOTDIR)/rules.make
