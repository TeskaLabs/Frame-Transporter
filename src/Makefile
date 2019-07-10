ROOTDIR=..
include $(ROOTDIR)/config.make

LIB=libft.a

OBJS=\
	boolean.o \
	base32.o \
	fd.o \
	ini.o \
	config.o \
	log.o \
	log_file.o \
	pubsub.o \
	sockaddr.o \
	mempool/frame.o \
	mempool/pool.o \
	mempool/zone.o \
	cols/list.o \
	cols/iphashmap.o \
	context.o \
	time-windows.o \
	watchers/timer-windows.o \
	winpipes/named_dgram.o \

#	sock/listener.o \
#	sock/stream.o \
#	sock/dgram.o \
#	proto/socks.o \

#	log_syslog.o \
#	daemon.o \

all: $(LIB)

coverage:
	gcov *.c
	gcov -o cols cols/*.c 
	gcov -o mempool mempool/*.c 
	gcov -o sock sock/*.c 


include $(ROOTDIR)/rules.make
