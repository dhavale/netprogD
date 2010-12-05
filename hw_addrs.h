/* Our own header for the programs that need hardware address info. */
#ifndef HW_ADDR_H
#define HW_ADDR_H

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <assert.h>
//#include <linux/if_arp.h>

#define ODR_K2159 2159

#define	IF_NAME		16	/* same as IFNAMSIZ    in <net/if.h> */
#define	IF_HADDR	 6	/* same as IFHWADDRLEN in <net/if.h> */
#define MAX_IF		6
#define	IP_ALIAS  	 1	/* hwa_addr is an alias */
#define ETH_HDRLEN	14

extern struct sockaddr_in eth0_ip;	

extern	int total_if_count;

struct hwa_info {
  char    if_name[IF_NAME];	/* interface name, null terminated */
  char    if_haddr[IF_HADDR];	/* hardware address */
  int     if_index;		/* interface index */
  short   ip_alias;		/* 1 if hwa_addr is an alias IP address */
  struct  sockaddr  *ip_addr;	/* IP address */
  struct  hwa_info  *hwa_next;	/* next of these structures */
};

struct hw_odr_info{
  char    if_name[IF_NAME];     /* interface name, null terminated */
  char    if_haddr[IF_HADDR];   /* hardware address */
  int     if_index;             /* interface index */
  struct  sockaddr_in  ip_addr;   /* IP address */

};

extern int staleness;
extern	struct hw_odr_info if_list[MAX_IF];
/* function prototypes */
struct hwa_info	*get_hw_addrs();
struct hwa_info	*Get_hw_addrs();
void	free_hwa_info(struct hwa_info *);


#endif

