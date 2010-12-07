#ifndef _TOUR_LIB_H
#define _TOUR_LIB_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <errno.h>
#include <netdb.h>

#include "common_lib.h"
#define MAX_PKT 1024
#define TOUR_K117 177
#define TOUR_K2160 2160
#define TOUR_ID  1


#ifdef ARP_DEBUG
#define dprintf(fmt, args...) printf(fmt, ##args)
#else
#define dprintf(fmt, args...)
#endif



extern struct in_addr eth0_ip;

struct tour_header{
unsigned long source;
unsigned long multicast;
int mport;
int index_in_tour;
int total_nodes;
};

char * get_name(unsigned long ip);


int areq(struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr);

int recv_process_tour_packet(int sockrt);

int send_tour_packet(int sockrt, unsigned long src_ip,unsigned long dest_ip , struct tour_header* th,void *buff, int len);
#endif
