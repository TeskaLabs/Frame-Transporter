BIN=httpserver

OBJS=main.o listen.o config.o connection.o http_parser.o

LDLIBS+=-L$(ROOTDIR)/src -lft

all: $(BIN)

ROOTDIR=../..
include $(ROOTDIR)/rules.make
