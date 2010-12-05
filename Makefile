
CC = gcc

LIBS = /home/sandeep/unpv13e/libunp.a -lc

FLAGS = -g -O2

all: get_hw_addrs.o prhwaddrs.o arp.o arp prhwaddrs
#	${CC} -o odr odr.o odr_lib.o get_hw_addrs.o ${LIBS}
#	${CC} -o odr_sender odr_sender.o odr_lib.o get_hw_addrs.o ${LIBS} 
#	${CC} -o client client.o time_lib.o ${LIBS}
#	${CC} -o server server.o time_lib.o ${LIBS}

prhwaddrs: get_hw_addrs.o prhwaddrs.o
	${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}

arp: get_hw_addrs.o arp.o
	${CC} -o arp arp.o get_hw_addrs.o ${LIBS}

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c

odr.o:	odr.c
	${CC} ${FLAGS} -c odr.c
arp.o: arp.c
	${CC} ${FLAGS} -c arp.c

#odr_sender.o: odr_sender.c
#	${CC} ${FLAGS} -c odr_sender.c

odr_lib.o: odr_lib.c
	${CC} ${FLAGS} -c odr_lib.c


time_lib.o: time_lib.c
	${CC} ${FLAGS} -c time_lib.c

client.o: client.c
	${CC} ${FLAGS} -c client.c

server.o: server.c
	${CC} ${FLAGS} -c server.c


clean:
	rm -rf *.o prhwaddrs odr odr_recv odr_sender server client 

