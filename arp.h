#ifndef ARP_H_
#define ARP_H_
#include "hw_addrs.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <errno.h>
#include <netdb.h>
#include "common_lib.h"


#ifdef ARP_DEBUG
#define dprintf(fmt, args...) printf(fmt, ##args)
#else
#define dprintf(fmt, args...)
#endif

#define ARP_K2159 2159
#define ARP_9512K 9512
#define LISTENQ 1024

struct ip_to_mac{
unsigned long ip;
unsigned char mac[IF_HADDR];
};

extern struct ip_to_mac self_list[MAX_IF];

extern int eth0_index;
extern struct sockaddr_in eth0_ip;
extern unsigned char eth0_mac[IF_HADDR];
extern int total_self_entries;

typedef struct arp_packet_type{
int type;
#define AREQ 0
#define AREP 1
int proto_id;
unsigned short hatype;
unsigned char src_mac[IF_HADDR];
unsigned long src_ip;
unsigned char dest_mac[IF_HADDR];
unsigned long dest_ip;
} t_arp;

typedef struct arp_cache_entry{
unsigned long ip;
int sll_ifindex;
unsigned char mac[IF_HADDR];
unsigned short sll_hatype;
int connfd;
int incomplete;
struct arp_cache_entry * next;
}cache_entry;

extern cache_entry * cache_head;


int recv_process_pf_packet(int sockfd);

int process_app_conn(int sockfd,int connfd);

char * get_name(unsigned long ip);



#endif

