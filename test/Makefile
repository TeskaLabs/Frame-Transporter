-include rules.site

BIN=unittest

OBJS=main.o testutils.o $(patsubst %.c,%.o,$(wildcard utest_*.c))

all: $(BIN)
	@echo " [UT]" $(BIN)
	@./$(BIN)

LDLIBS+=-lcheck -L$(ROOTDIR)/src -lft -lpthread

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDLIBS += -lrt
endif

ROOTDIR=..
include $(ROOTDIR)/rules.make
