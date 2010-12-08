#ifndef _COMMON_LIB_H
#define _COMMON_LIB_H


#define UNIX_D_PATH "/tmp/arp2159.dg"

struct hwaddr{
int sll_ifindex;
unsigned short sll_hatype;
unsigned char sll_halen;
unsigned char sll_addr[8];
};


#endif
