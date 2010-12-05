#ifndef ARP_LIB_H
#define ARP_LIB_H

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

struct hwaddr{
int sll_ifindex;
unsigned short sll_hatype;
unsigned char sll_halen;
unsigned char sll_addr[8];
};

int add_cache_entry(unsigned long ip, int if_index,unsigned char *mac,unsigned short sll_hatype,int connfd);
#endif
