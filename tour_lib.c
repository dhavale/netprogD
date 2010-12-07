#include <stdio.h>
#include <stdlib.h>
#include "tour_lib.h"

char * get_name(unsigned long ip)
{
        struct hostent * host;
        host= gethostbyaddr(&ip,4,AF_INET);
	struct in_addr *in = (struct in_addr*)&ip;
        dprintf("\nfor ip %s hname is %s\n",(char *)inet_ntoa(*in),host->h_name);
        return host->h_name;
}

int areq(struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr)
{
	int ret=0;
	struct sockaddr_un servaddr;	
	int sockfd;
	ssize_t len=0;
	struct sockaddr_in *in;
	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if(sockfd<0)
        {
                perror("unable to create socket:");
                exit(EXIT_FAILURE);
        }

	bzero(&servaddr, sizeof(servaddr)); /* servaddr address for us */
       	servaddr.sun_family = AF_LOCAL;
       	strcpy(servaddr.sun_path,UNIX_D_PATH);

	if(connect(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr))<0)
	{
		perror("Connect error..:");
		return -1;
	}

	in = (struct sockaddr_in *)IPaddr;
	len = write(sockfd,&in->sin_addr,4);
	if(len!=4)
	{
		perror("write failed:");
		return -1;
	}

	len=read(sockfd,HWaddr,sizeof(struct hwaddr));
	if(len<= 0)
	{
		printf("len=%d sizeof=%d\n",len,sizeof(struct hwaddr));
		perror("read failed:");
		return -1;	
	}

	return ret;
}



int send_tour_packet(int sockrt, unsigned long src_ip,unsigned long dest_ip , struct tour_header* th,void *payload, int len)
{
	void *sendbuff = malloc(MAX_PKT);
	struct sockaddr_in dest;
	int ret=0;
	struct ip *ipheader = (struct ip *)sendbuff;
	unsigned char *data;

	memset(sendbuff,0,MAX_PKT);
	memset(&dest,0,sizeof(struct sockaddr_in));

	ipheader->ip_hl = 5;
	ipheader->ip_v = 4;
	ipheader->ip_len = (sizeof(struct ip) + sizeof(struct tour_header) + len);
	ipheader->ip_id = htons(TOUR_ID);
	ipheader->ip_ttl =1;
	ipheader->ip_p = TOUR_K117;
	ipheader->ip_sum=0;
	ipheader->ip_src.s_addr = src_ip;
	ipheader->ip_dst.s_addr = dest_ip;
	
	dest.sin_addr.s_addr = dest_ip;
	dest.sin_family = AF_INET;

	memcpy(sendbuff+sizeof(struct ip),(void*)th,sizeof(struct tour_header));
	data = sendbuff+ sizeof(struct ip) + sizeof(struct tour_header);

	memcpy(data,payload,len);

	ret= sendto(sockrt,sendbuff, ipheader->ip_len, 0, (struct sockaddr *)&dest, sizeof(dest) );

	if(ret!= ipheader->ip_len)
	{
		perror("sendto failed:");
		ret=-1;
	}
	
	else ret=0;	
		
	free(sendbuff);
	return ret;
				

}

int recv_process_tour_packet(int sockrt)
{
	void * recvbuff = malloc(MAX_PKT);
	struct sockaddr_in cliaddr;
	int clilen = sizeof(cliaddr);
	int ret=0,payload_len=0;
	struct tour_header *th;
	unsigned long *data;
	memset(&cliaddr,0,sizeof(struct sockaddr_in));	
	memset(recvbuff,0,MAX_PKT);
	
	unsigned long next_hop;

	ret= recvfrom(sockrt,recvbuff,MAX_PKT, 0,(struct sockaddr*)&cliaddr,&clilen);

	if(ret<=0)
	{
		perror("recvfrom:");
		free(recvbuff);
		return -1;
	}
	
	payload_len = ret- (sizeof(struct tour_header)+(sizeof(struct ip)));

	th = (struct tour_header *) (recvbuff+sizeof(struct ip));

	data = (recvbuff + (sizeof(struct tour_header))+ sizeof(struct ip));

	/*start pinging to source node,if have not done before*/
	/*register to multicast if havent done before*/
	printf("index=%d total=%d\n",th->index_in_tour,th->total_nodes);

	if(th->index_in_tour == (th->total_nodes))
	{
		/*tour ends at you*/
		printf("%s: I am the last node\n",(char *)get_name(eth0_ip.s_addr));
		ret=0;
	}		
	else{
		
		printf("%s: I am part of the tour",(char *)get_name(eth0_ip.s_addr));
		th->index_in_tour++;
		next_hop = data[th->index_in_tour-1];
		printf("next hop is: %s\n",(char *)get_name(next_hop));
		ret = send_tour_packet(sockrt,eth0_ip.s_addr,next_hop,th,data,payload_len);
		if(ret<0)
		{
			printf("Sending to next hop failed.\n");
		}
		else ret=0;
		/*send tour packet to next_hop.*/
			
	}
	
	free(recvbuff);

	return ret;		
}
