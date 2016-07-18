-include rules.site

BIN=unittest

OBJS=main.o testutils.o $(patsubst %.c,%.o,$(wildcard utest_*.c))

all: $(BIN)
	@echo " [UT]" $(BIN)
	@./$(BIN)
ifdef USE_COVERAGE
	gcov main.c
endif

LDLIBS+=-lcheck -L$(ROOTDIR)/src -lsccmn

ROOTDIR=..
include $(ROOTDIR)/rules.make
