#include "common_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/un.h>
#include <netdb.h>
#include <assert.h>
#include "tour_lib.h"

struct in_addr eth0_ip;

int main(int argc,char*argv[])
{
	
	struct hostent *he;
	struct sockaddr_un cliaddr,servaddr;
	struct in_addr *req=NULL;
	struct hwaddr retaddr;
	char client_name[52]={};
	int len,j;
	struct sockaddr_in IPaddr;
	unsigned long *payload;
	struct in_addr *temp_ip;
	struct hostent *hp;
	char source_name[20] = {};
	
	int sockrt,one=1,payload_len,i;
	const int *val =&one;

	struct tour_header th;


	memset(&th,0,sizeof(th));
	
	memset(&IPaddr,0,sizeof(IPaddr));	
	memset(&retaddr,0,sizeof(retaddr));

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
		perror("socket failed:");
		exit(EXIT_FAILURE);
	}	
	
	if(setsockopt(sockrt,IPPROTO_IP,IP_HDRINCL,val,sizeof(one))<0)
	{
		perror("setsockopt error:");
		exit(EXIT_FAILURE);
	}		
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
		/**fill in tour header*/
			th.source = eth0_ip.s_addr;
			th.index_in_tour = 1;
			th.total_nodes =argc-1;

			send_tour_packet(sockrt,th.source,payload[0],&th,payload,payload_len);
		
	}

	while(1)
	{
		recv_process_tour_packet(sockrt);
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
