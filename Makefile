
CC = gcc

LIBS = /home/sandeep/unpv13e/libunp.a -lc

FLAGS = -g -O0

all: get_hw_addrs.o prhwaddrs.o arp.o tour.o tour_lib.o arp prhwaddrs tour
#	${CC} -o odr odr.o odr_lib.o get_hw_addrs.o ${LIBS}
#	${CC} -o odr_sender odr_sender.o odr_lib.o get_hw_addrs.o ${LIBS} 
#	${CC} -o client client.o time_lib.o ${LIBS}
#	${CC} -o server server.o time_lib.o ${LIBS}

prhwaddrs: get_hw_addrs.o prhwaddrs.o
	${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}

arp: get_hw_addrs.o arp.o arp_lib.o
	${CC} -o arp arp.o arp_lib.o get_hw_addrs.o ${LIBS}

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c

tour: tour_lib.o tour.o
	${CC} -o tour tour_lib.o tour.o ${LIBS}

tour.o:  tour.c
	${CC} ${FLAGS} -c tour.c

tour_lib.o:
	${CC} ${FLAGS} -c tour_lib.c


arp.o: arp.c
	${CC} ${FLAGS} -c arp.c

arp_lib.o: arp_lib.c
	${CC} ${FLAGS} -c arp_lib.c

clean:
	rm -rf *.o prhwaddrs arp tour *.dg

