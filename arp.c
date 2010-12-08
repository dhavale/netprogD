#include "arp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hw_addrs.h"

struct ip_to_mac self_list[MAX_IF];

int total_self_entries=0;
struct sockaddr_in eth0_ip;
int eth0_index;

unsigned char eth0_mac[IF_HADDR]={};

void arp_init()
{


	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	struct sockaddr_in *si;
	char   *ptr;
	int    i, prflag;

	printf("\n");

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		
		printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		if(!strcmp(hwa->if_name,"eth0")||(hwa->ip_alias == IP_ALIAS))
		{
			si= (struct sockaddr_in* )hwa->ip_addr;
			self_list[total_self_entries].ip = si->sin_addr.s_addr;
			memcpy(self_list[total_self_entries].mac,hwa->if_haddr,IF_HADDR);
			if(!(strcmp(hwa->if_name,"eth0")))
			{
				eth0_index = hwa->if_index;
				eth0_ip= *(struct sockaddr_in*) hwa->ip_addr;
				memcpy(eth0_mac,hwa->if_haddr,IF_HADDR);
			}
			total_self_entries++;
	
		}
		if ( (sa = hwa->ip_addr) != NULL)
			printf("         IP addr = %s\n", (char *)Sock_ntop_host(sa, sizeof(*sa)));
				
		prflag = 0;
		i = 0;
		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf("         HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		printf("\n         interface index = %d\n\n", hwa->if_index);
	}

//	memset(&eth0,0,sizeof(struct sockaddr_in));

	printf("\n[INFO]eth0 ip recognized %s \n",(char*)inet_ntoa(eth0_ip.sin_addr));

	free_hwa_info(hwahead);

}

int main()
{
	int i=0,j=0;
	int maxfdp;

        fd_set rset;

        int sockfd; /*socketdescriptor*/
	int connfd;
        int domainfd;
        struct sockaddr_un servaddr,cliaddr;
	int clilen = sizeof(cliaddr);

	arp_init();
	/*printing self Table*/
	printf("\nTotal %d entries in self table\n",total_self_entries);

	for(i=0;i<total_self_entries;i++)
	{
		printf("ip:%s mac: ",(char *)inet_ntoa(*(struct in_addr*)(&self_list[i].ip)));
	
		for(j=0;j<IF_HADDR;j++)
		{
			printf("%.2x:",self_list[i].mac[j]);
		}
		printf("\b \n");


	}
	unlink(UNIX_D_PATH);

        bzero(&servaddr,sizeof(servaddr));
        servaddr.sun_family= AF_LOCAL;
        strcpy(servaddr.sun_path,UNIX_D_PATH);

        domainfd= socket(AF_LOCAL,SOCK_STREAM,0);

        if (domainfd == -1) {
                perror("Unable to create socket:");
                exit(EXIT_FAILURE);
        }

	sockfd = socket(PF_PACKET, SOCK_RAW, htons(2159));

       if (sockfd == -1) {
                perror("Unable to create socket:");
                exit(EXIT_FAILURE);
        }


        if(bind(domainfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
	{
		perror("bind error on domainfd:");
		exit(EXIT_FAILURE);
	}

	if(listen(domainfd,LISTENQ)<0)
	{
			
		perror("listen error on domainfd:");
		exit(EXIT_FAILURE);
	}
	
	FD_ZERO(&rset);

        while(1)
        {

                FD_ZERO(&rset);
                FD_SET(sockfd,&rset);
                FD_SET(domainfd,&rset);
                if(sockfd>domainfd)
                        maxfdp=sockfd+1;
                else
                        maxfdp=domainfd+1;



                select(maxfdp,&rset,NULL,NULL,NULL);
                if(FD_ISSET(sockfd,&rset))
                {
			
                        recv_process_pf_packet(sockfd);
                }
                if(FD_ISSET(domainfd,&rset))
                {
			connfd = accept(domainfd,(struct sockaddr *)&cliaddr,&clilen);
			if(errno==EINTR)
				continue;
			if(connfd<0)
			{
				perror("accept error:");
				continue;
			}	
                        process_app_con(sockfd,connfd);
                }
        }
 
	return 0;
}
