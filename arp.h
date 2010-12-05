#ifndef ARP_H_
#define ARP_H_
#include "hw_addrs.h"

struct ip_to_mac{
unsigned long ip;
unsigned char mac[IF_HADDR];
};

extern struct ip_to_mac self_list[MAX_IF];

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

#endif

