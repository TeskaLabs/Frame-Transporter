LIB=libft.a

OBJS=\
	boolean.o \
	base32.o \
	fd.o \
	ini.o \
	config.o \
	log.o \
	log_file.o \
	log_syslog.o \
	pubsub.o \
	exit.o \
	heartbeat.o \
	daemon.o \
	sockaddr.o \
	sock/listener.o \
	sock/stream.o \
	sock/dgram.o \
	mempool/frame.o \
	mempool/pool.o \
	mempool/zone.o \
	context.o \
	cols/list.o \
	cols/iphashmap.o \
	proto/socks.o \

all: $(LIB)

coverage:
	gcov *.c
	gcov -o cols cols/*.c 
	gcov -o mempool mempool/*.c 
	gcov -o sock sock/*.c 

ROOTDIR=..
include $(ROOTDIR)/rules.make
