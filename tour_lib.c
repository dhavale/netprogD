#include <stdio.h>
#include <stdlib.h>
#include "tour_lib.h"
#include "hw_addrs.h"


unsigned char eth0_mac[IF_HADDR];
int eth0_index;
short int icmp_sequence=0;


unsigned short checkSum(unsigned char *buff, int iSize);

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
	ipheader->ip_len = htons(ipheader->ip_len);
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

	ret= sendto(sockrt,sendbuff, ntohs(ipheader->ip_len), 0, (struct sockaddr *)&dest, sizeof(dest) );

	if(ret!= ntohs(ipheader->ip_len))
	{
		perror("sendto failed:");
		ret=-1;
	}
	
	else ret=0;	
		
	free(sendbuff);
	return ret;
				

}

int recv_process_tour_packet(int sockrt,int sockicmp)
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
	send_echo_req(sockicmp,th->source);

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

int send_echo_req(int sockicmp, unsigned long dest_ip)
{
	struct sockaddr_ll socket_dest_address;
        unsigned char* src_mac=NULL, *dest_mac =NULL;
        int i=0,j=0,total_len=0;

        int if_index;
        /*buffer for ethernet frame*/
        void* buffer = (void*)malloc(MAX_PKT);

        /*pointer to ethenet header*/
        unsigned char* etherhead = buffer;

        unsigned char* iphead = buffer + ETH_HDRLEN;

	unsigned char* icmphead = buffer+ ETH_HDRLEN + sizeof(struct ip);

        /*another pointer to ethernet header*/
        struct ethhdr *eh = (struct ethhdr *)etherhead;

	struct ip *iph = (struct ip *) iphead;
	
	
	struct icmp *icmph = (struct icmp *) icmphead;
 
	unsigned char * data = icmphead+ sizeof(struct icmp);

	struct sockaddr_in IPaddr;
	struct hwaddr retaddr;
	
        int send_result = 0;
        unsigned char ch;


        src_mac= eth0_mac;
        if_index= eth0_index;

	memset(buffer,0,MAX_PKT);
	memset(&IPaddr,0,sizeof(struct sockaddr_in));
	memset(&retaddr,0,sizeof(struct hwaddr));

	IPaddr.sin_addr.s_addr = dest_ip;

	if(areq((struct sockaddr*)&IPaddr,sizeof(struct sockaddr_in),&retaddr)<0)
        {
                printf("Areq() failed..");
                return -1;
        }
	dest_mac = retaddr.sll_addr;
	
        printf("Sending frame hdr src: ");
        for(j=0;j<6;j++)
        {
           ch=src_mac[j];
                printf("%.2x:",ch);
        }

        printf("  dest: ");

        for(j=0;j<6;j++)
        {
                ch=dest_mac[j];
                printf("%.2x:",ch);
        }

        printf(" dest %s\n",get_name(dest_ip));



        memset(&socket_dest_address,0,sizeof(socket_dest_address));
        /*prepare sockaddr_ll*/

        /*RAW communication*/
 	socket_dest_address.sll_family   = PF_PACKET;
        /*we don't use a protocoll above ethernet layer
          ->just use anything here*/
        socket_dest_address.sll_protocol = htons(ETH_P_IP);


        /*index of the network device
        see full code later how to retrieve it*/
        socket_dest_address.sll_ifindex  = if_index;

        /*address length*/
        socket_dest_address.sll_halen    = ETH_ALEN;
        /*MAC - begin*/
        memcpy(socket_dest_address.sll_addr,dest_mac,ETH_ALEN);
        /*MAC - end*/

        /*set the frame header*/
        memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
        memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
        eh->h_proto = htons(ETH_P_IP);
        /*fill the frame with some data*/
        /*for (j = 0; j < 1500; j++) {
                data[j] = (unsigned char)((int) (255.0*rand()/(RAND_MAX+1.0)));
        }
        */

	/*
		Fill IP header
	*/
	iph->ip_hl = 5;
	iph->ip_v = 4;
	iph->ip_len = sizeof(struct ip) + sizeof(struct icmp)+56;
	iph->ip_len = htons(iph->ip_len);
	iph->ip_id = TOUR_ID+1;
	iph->ip_ttl = 1;
	iph->ip_p = IPPROTO_ICMP;
	 	
	iph->ip_sum = 0; /* to be calculated*/

	iph->ip_src.s_addr = eth0_ip.s_addr;
	iph->ip_dst.s_addr = dest_ip;

	/*
		Fill icmp header
	*/

	icmph->icmp_type = 8; /*echo request*/	
	icmph->icmp_cksum =0; /* to be calculated*/
	icmph->icmp_id = htons(TOUR_K2160);
	icmph->icmp_seq = htons(++icmp_sequence); 	

	strcpy(data,"Parag-sandeep");

	icmph->icmp_cksum= checkSum(icmphead,sizeof(struct icmp)+56);

	iph->ip_sum = checkSum(iphead,sizeof(struct ip)+sizeof(struct icmp)+56);

	total_len = ETH_HDRLEN + sizeof(struct ip) + sizeof(struct icmp) + 56;
			
		
        /*send the packet*/
        send_result = sendto(sockicmp, buffer, total_len, 0,
                      (struct sockaddr*)&socket_dest_address, sizeof(socket_dest_address));
        if (send_result != total_len) {
                perror("Send failed..:");
                exit(EXIT_FAILURE);
        }
	printf("\n echo request sent..");

	free(buffer);

        return 0;
			

}

int tour_init()
{
	struct hwa_info *hwa, *hwahead;
	int i=0;

	for(hwahead = hwa = Get_hw_addrs();hwa != NULL; hwa = hwa->hwa_next){
		
		if(!strcmp(hwa->if_name,"eth0"))
		{
			memcpy(eth0_mac,hwa->if_haddr,IF_HADDR);
			eth0_index = hwa->if_index;
			break;
		}
	}
	printf("\neth0_mac: ");
	for(i=0 ; i < IF_HADDR; i++)
	{
		printf("%.2x:",eth0_mac[i]);	
	}
	printf("\b \n");

	free_hwa_info(hwahead);
	return 0;
}

unsigned short checkSum(unsigned char *buff, int iSize)
{
	unsigned short oddByte,*ckbuff;
	int sum;
	
	ckbuff = (unsigned short *) buff;
	sum = 0;
//	printf("sum is %x and %d bytes remaining \n\r", sum,iSize);
	while(iSize > 1)
	{
	
		sum += *ckbuff++;
		iSize -= 2;
//	    printf("sum is %x and %d bytes remaining \n\r", sum,iSize);
	}
 	
	if( iSize == 1 )
	{
		oddByte =0;
		*((unsigned short *) &oddByte) = *(unsigned short *) ckbuff;
		iSize--;
	
	}
	
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	
	return (unsigned short) ~sum;
	
 
}


int recv_echo_reply(int sockpg)
{
	void *recvbuff = malloc(MAX_PKT);
	struct sockaddr_in cliaddr;
	int clilen = sizeof(cliaddr);
	int ret=0;
	struct icmp *icmph;
	unsigned char * icmphead;
	struct ip *iph = (struct ip *)recvbuff;
	unsigned char * data;
	memset(&cliaddr,0,sizeof(struct sockaddr_in));	
	memset(recvbuff,0,MAX_PKT);
	
	ret = recvfrom(sockpg,recvbuff,MAX_PKT, 0,(struct sockaddr*)&cliaddr,&clilen);
	if(ret<=0)
	{
		perror("recvfrom:");
		free(recvbuff);
		return -1;
	}

	icmphead = (unsigned char *) (recvbuff+ sizeof(struct ip));

	icmph = (struct icmp *)icmphead;				

	data = icmphead + sizeof(struct icmp);
	printf("icmp id n: %d h:%d \n",icmph->icmp_id,ntohs(icmph->icmp_id));

	if(ntohs(icmph->icmp_id)==(TOUR_K2160))
	{
		printf("\n");
		if(icmph->icmp_type==8)
			printf("echo request");
		else if(icmph->icmp_type==0)
			printf("echo reply");

		printf(" from: %s ",get_name(iph->ip_src.s_addr));
		printf(" seq=%d data=%s\n",ntohs(icmph->icmp_seq),data);	
	}	
	
	free(recvbuff);
	return 0;
	
}
