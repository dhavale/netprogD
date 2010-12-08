#include "common_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/un.h>
#include <netdb.h>
#include <assert.h>
#include "tour_lib.h"

struct in_addr eth0_ip;

struct ip_mreq imreq;

int main(int argc,char*argv[])
{
	
	struct hostent *he;
	struct in_addr *req=NULL;
	struct hwaddr retaddr;
	char client_name[52]={};
	int len,j;
	struct sockaddr_in IPaddr;
	unsigned long *payload;
	struct in_addr *temp_ip;
	struct hostent *hp;
	char source_name[20] = {};
	
	struct timeval tv1;
	int lsockmc;	
	int sockrt,one=1,payload_len,i;
	int sockicmp, sockpg,maxfdp;

	const int *val =&one;
	int tsockfd;

	struct sockaddr_in multiaddr;
	socklen_t multilen = sizeof(multiaddr);

	struct tour_header th;
	fd_set rset;
	void *recvmulti = malloc(200);
	void *multimsg = malloc(200);
	u_int yes=1;
	struct in_addr iaddr;
	struct sockaddr_in recvaddr;
	struct sockaddr_in servaddr;

	memset(&servaddr,0,sizeof(servaddr));
	memset(&recvaddr,0,sizeof(recvaddr));	
	memset(&multiaddr,0,sizeof(multiaddr));	
	memset(&th,0,sizeof(th));
	memset(&imreq,0,sizeof(imreq));	
	memset(&IPaddr,0,sizeof(IPaddr));	
	memset(&retaddr,0,sizeof(retaddr));

	tour_init();

	gethostname(source_name,sizeof(source_name));

	printf("source: %s\n",source_name);	
		
	hp = gethostbyname(source_name);


	if(hp==NULL)
	{
		perror("gethostbyname:");
		exit(EXIT_FAILURE);
	}
	eth0_ip  = *(struct in_addr *)hp->h_addr;

	sockrt = socket(PF_INET,SOCK_RAW,TOUR_K117);
	
	if(sockrt<0)
	{
		perror("sockrt failed:");
		exit(EXIT_FAILURE);
	}	

	sockpg = socket(PF_INET,SOCK_RAW,IPPROTO_ICMP);

	if(sockpg<0){

		perror("sockpg failed:");
		exit(EXIT_FAILURE);
	}

	sockicmp= socket(PF_PACKET,SOCK_RAW,ETH_P_IP);
		
	if(sockicmp<0){

		perror("sockicmp failed:");
		exit(EXIT_FAILURE);
	}
	
	if(setsockopt(sockrt,IPPROTO_IP,IP_HDRINCL,val,sizeof(one))<0)
	{
		perror("setsockopt error:");
		exit(EXIT_FAILURE);
	}		

	/*open a multicast socket*/

	multiaddr.sin_family = AF_INET;
	multiaddr.sin_port = htons(MULTICAST_PORT);
	multiaddr.sin_addr.s_addr = htonl(INADDR_ANY);


	lsockmc = socket(AF_INET,SOCK_DGRAM,0);
	if(lsockmc<0)
	{
		perror("lsock multicast:");
		exit(EXIT_FAILURE);
	}
	if(setsockopt(lsockmc,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes))<0)
	{
		perror("setsockopt reuseaddr lsockmc");
		exit(EXIT_FAILURE);
	}
	if(bind(lsockmc,(struct sockaddr*)&multiaddr,multilen)<0)
	{
		perror("bind:");
		exit(EXIT_FAILURE);
	}	

#if 0 

	iaddr.s_addr = INADDR_ANY;
	if(setsockopt(sockmc,IPPROTO_IP,IP_MULTICAST_IF,&iaddr,sizeof(iaddr))<0)
	{
		perror("setsockopt iaddr:");
		exit(EXIT_FAILURE);
	}

#endif
	if(argc>1)
	{
		payload_len = (argc-1)* sizeof(unsigned long);

		payload = (unsigned long*)malloc(payload_len);

		memset(payload,0,payload_len);
	
		for(i=0;i<argc-1;i++)
		{
			printf("node %d : %s\n",i,argv[i+1]);

			hp = gethostbyname(argv[i+1]);
			
			if(hp==NULL||(!strcmp(argv[i+1],source_name)))
			{
				printf("Bad tour node %s!\n",argv[i+1]);
				exit(EXIT_FAILURE);
			}
			
			temp_ip = (struct in_addr*)hp->h_addr;
			payload[i] = temp_ip->s_addr;
		}	
			/*server, starts from you, do multicast setup*/
			imreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP);
			imreq.imr_interface.s_addr = htonl(INADDR_ANY);
 	
			if(setsockopt(lsockmc,IPPROTO_IP,IP_ADD_MEMBERSHIP,&imreq,sizeof(imreq))<0)
			{
				perror("multicast sockopt:");
				exit(EXIT_FAILURE);
			}
		/**fill in tour header*/
			
			th.source = eth0_ip.s_addr;
			th.index_in_tour = 1;
			th.total_nodes =argc-1;
			th.multicast = imreq.imr_multiaddr.s_addr;
			th.mport = htons(MULTICAST_PORT);
			send_tour_packet(sockrt,th.source,payload[0],&th,payload,payload_len);
		
	}

	FD_ZERO(&rset);
	int count=total_replies -1;
	time_t ticks;
	while(1)
	{
		FD_SET(sockpg,&rset);
	
		FD_SET(sockrt,&rset);
		
		FD_SET(lsockmc,&rset);
		
		tv1.tv_sec = 1;
		tv1.tv_usec = 0;
	
		memset(recvmulti,0,200);
		multilen = sizeof(recvaddr);

		if(sockpg<sockrt)
			maxfdp = sockrt+1;
		else
			maxfdp = sockpg+1;

		if(maxfdp<=lsockmc)
			maxfdp = lsockmc+1;
		
		if(joined && (time(NULL)>ticks))
		{
			count=total_replies;
			ticks = time(NULL);
			send_echo_req(sockicmp,tour_source);
		}
		select(maxfdp,&rset,NULL,NULL,&tv1);
		if(FD_ISSET(sockrt,&rset))
		{	
			recv_process_tour_packet(sockrt,sockicmp,lsockmc,sockpg);
		}
		if(FD_ISSET(sockpg,&rset))
		{
			recv_echo_reply(sockpg);
		}
		if(FD_ISSET(lsockmc,&rset))
		{
			recvaddr.sin_family = AF_INET;
			recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			recvaddr.sin_port  = htons(MULTICAST_PORT);

			if(recvfrom(lsockmc,recvmulti,200,0,(struct sockaddr*)&recvaddr,&multilen)<0)
			{
				perror("recvfrom multicast:");
				exit(EXIT_FAILURE);
			}
			printf("Node %s. Received:%s\n",get_name(eth0_ip.s_addr),(char*)recvmulti);

			tsockfd = socket(AF_INET,SOCK_DGRAM,0);
			if(tsockfd<0)
			{
				perror("tsockfd:");
				exit(EXIT_FAILURE);
			}
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(MULTICAST_PORT);
			servaddr.sin_addr.s_addr = inet_addr(MULTICAST_IP);
			
			sprintf(multimsg,"Node %s. I am a member of the group.",get_name(eth0_ip.s_addr));
			if(sendto(tsockfd,multimsg,strlen(multimsg)+1,0,(struct sockaddr*)&servaddr,sizeof(servaddr))<=0)
			{
				perror("sendto multicast failed:");
			}
			printf("Node %s. Sending: %s\n",get_name(eth0_ip.s_addr),(char*)multimsg);

			close(tsockfd);
			break;	
		}
	}

	FD_ZERO(&rset);
	while(1)
	{
		FD_SET(lsockmc,&rset);
		memset(recvmulti,0,200);	
		tv1.tv_sec=5;
		tv1.tv_usec = 0;
		
		select(lsockmc+1,&rset,NULL,NULL,&tv1);

		if(FD_ISSET(lsockmc,&rset))
		{
			recvaddr.sin_family = AF_INET;
			recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			recvaddr.sin_port  = htons(MULTICAST_PORT);

			if(recvfrom(lsockmc,recvmulti,200,0,(struct sockaddr*)&recvaddr,&multilen)<0)
			{
				perror("recvfrom multicast:");
				exit(EXIT_FAILURE);
			}
			printf("Node %s. Received:%s\n",get_name(eth0_ip.s_addr),(char*)recvmulti);
	
		}
		else{
			printf("Gracefully exiting tour application..\n");
			break;
		}
		
	}
/*
	he=gethostbyname(argv[1]);
	if(he==NULL)
	{
		printf("bad name!!\n");
		return -1;
	}
	
	memcpy(&IPaddr.sin_addr,he->h_addr,sizeof(struct in_addr));

	if(areq((struct sockaddr*)&IPaddr,sizeof(struct sockaddr_in),&retaddr)<0)
	{
		printf("Areq() failed..");
		return -1;
	}
*/

#if 0
		int sockfd;
       sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if(sockfd<0)
        {
                perror("unable to create socket:");
                exit(EXIT_FAILURE);
        }

        gethostname(client_name,sizeof(client_name));

       bzero(&servaddr, sizeof(servaddr)); /* servaddr address for us */
       servaddr.sun_family = AF_LOCAL;
       strcpy(servaddr.sun_path,UNIX_D_PATH);

	if(connect(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr))<0)
	{
		perror("Connect error..:");
		return -1;
	}
	req = (struct in_addr *)he->h_addr_list[0];

	printf("arp for ip: %s",(char*)inet_ntoa(*req));

	len = write(sockfd,req,4);
	if(len!=4)
	{
		perror("write failed:");
		return -1;
	}

	len=read(sockfd,&retaddr,sizeof(retaddr));
	if(len!= sizeof(retaddr))
	{
		perror("read failed:");
		return -1;	
	}
#endif
/*	
	printf("mac for %s :\n ",argv[1]);
	for(j=0;j<retaddr.sll_halen;j++)
	{
		printf("%.2x:",retaddr.sll_addr[j]);
	}	
	printf("\b \n");
*/

}
