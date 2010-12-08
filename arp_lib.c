#include "arp.h"

cache_entry *cache_head=NULL;
cache_entry * search_arp(unsigned long request_ip);


int add_cache_entry(unsigned long ip, int if_index,unsigned char *mac,unsigned short sll_hatype,int connfd);

int update_cache_entry(cache_entry *entry,unsigned long ip, int if_index,unsigned char *mac,unsigned short sll_hatype,int connfd);


char * get_name(unsigned long ip)
{
        struct hostent * host;
        host= gethostbyaddr(&ip,4,AF_INET);
	struct in_addr *in = (struct in_addr*)&ip;
        dprintf("\nfor ip %s hname is %s\n",(char *)inet_ntoa(*in),host->h_name);
        return host->h_name;
}


int send_arp_packet(int sockfd,unsigned char *dest_mac, t_arp *arp_packet)
{
        /*target address*/
        struct sockaddr_ll socket_dest_address;
        char src_mac[IF_HADDR]={};
        int i=0,j=0;
        int if_index=eth0_index;
        /*buffer for ethernet frame*/
        void* buffer = (void*)malloc(ETH_HDRLEN+sizeof(t_arp));

        /*pointer to ethenet header*/
        unsigned char* etherhead = buffer;

        /*userdata in ethernet frame*/
        unsigned char* data = buffer + ETH_HDRLEN;

        /*another pointer to ethernet header*/
        struct ethhdr *eh = (struct ethhdr *)etherhead;

        int send_result = 0;
        unsigned char ch;

	memcpy(src_mac,eth0_mac,IF_HADDR);
/*
        print ARP details
*/
        printf("ARP at node %s:",get_name(eth0_ip.sin_addr.s_addr));

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

        printf("\n\t ARP msg   type %s  src %s  ",(arp_packet->type==AREQ)?"AREQ":"AREP",get_name(arp_packet->src_ip));
        printf(" dest %s\n",get_name(arp_packet->dest_ip));

        if(arp_packet->type==AREQ)
        dprintf("\nSending AREQ ");
        else if(arp_packet->type==AREP)
        dprintf("\nSending AREP ");
        else dprintf("\nSending undefined ");

        memset(&socket_dest_address,0,sizeof(socket_dest_address));
        /*prepare sockaddr_ll*/

        /*RAW communication*/
	socket_dest_address.sll_family   = PF_PACKET;
        /*we don't use a protocoll above ethernet layer
          ->just use anything here*/
        socket_dest_address.sll_protocol = htons(ARP_K2159);

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
        eh->h_proto = htons(ARP_K2159);
        /*fill the frame with some data*/

        memcpy(data,arp_packet,sizeof(t_arp));

        /*send the packet*/
        send_result = sendto(sockfd, buffer, ETH_HDRLEN+sizeof(t_arp), 0,
                      (struct sockaddr*)&socket_dest_address, sizeof(socket_dest_address));
        if (send_result == -1) {
                perror("Send failed..:");
                exit(EXIT_FAILURE);
        }
        return 0;
                                            
}


int recv_process_pf_packet(int sockfd)
{
        void* recv_buffer = (void*)malloc(ETH_HDRLEN+sizeof(t_arp)); /*Buffer for ethernet frame*/
        struct sockaddr_ll from;
        int size = sizeof(struct sockaddr_ll);
        t_arp *arp_packet;
        memset(&from,0,sizeof(from));
        unsigned char src_mac[IF_HADDR];
        unsigned char dest_mac[IF_HADDR];
        int length = 0; /*length of the received frame*/

        length = recvfrom(sockfd, recv_buffer,ETH_HDRLEN+sizeof(t_arp) , 0, (struct sockaddr*)&from,&size);

        if (length == -1) {
                perror("Recieve failed:");
                return -1;
        }

        arp_packet= (t_arp*)(recv_buffer+ETH_HDRLEN);
        memcpy(src_mac, (unsigned char *) (recv_buffer+IF_HADDR),IF_HADDR);
        memcpy(dest_mac,(unsigned char*)recv_buffer,IF_HADDR);


        switch(arp_packet->type)
        {
                case AREQ:
                        process_AREQ(sockfd,src_mac,from.sll_ifindex,arp_packet);

                break;
                case AREP:

                        dprintf("RREP from %x:%x:%x:%x:%x:%x to me at %x:%x:%x:%x:%x:%x \n",
                                src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5],
                              dest_mac[0],dest_mac[1],dest_mac[2],dest_mac[3],dest_mac[4],dest_mac[5]);
                        process_AREP(sockfd,src_mac,from.sll_ifindex,arp_packet);
                break;
                default:
                        printf("Garbage packet on ARP.. dropping\n");
                break;
        }



//      printf("data recieved: %s",(char*)recv_buffer+14);

        free(recv_buffer);
        return 0;

}

int  process_AREQ(int sockfd,unsigned char *src_mac,int from_index,t_arp* arp_packet)
{
	/*
		1. check if UID matches, drop otherwise
		2. see if packet is meant for you, reply or drop accordingly.
		3. Check the source ip, update the entry if it already exists.	
			
	*/

	cache_entry * entry = NULL;
	unsigned long src_ip = arp_packet->src_ip;
	t_arp reply;
	memset(&reply,0,sizeof(reply));
	int i=0;

	if(arp_packet->proto_id!=ARP_9512K)
	{
		printf("somebody with same proto field trying to mess up X-( dropping..\n");
		return -1;
	}	

	entry = search_arp(src_ip);

	if(entry)
	{	/*update source information if necessary*/
		update_cache_entry(entry,src_ip,eth0_index,arp_packet->src_mac,arp_packet->hatype,-1);
	}

	for(i=0;i<total_self_entries;i++)
	{
		if(self_list[i].ip== arp_packet->dest_ip)
			break;
	}

	if(i<total_self_entries)
	{
		/*you are the dest, send AREP*/
		reply.type = AREP;
		reply.proto_id = ARP_9512K;
		reply.hatype = 1;
		memcpy(reply.src_mac,self_list[i].mac,IF_HADDR);
		reply.src_ip = self_list[i].ip;
		memcpy(reply.dest_mac,arp_packet->src_mac,IF_HADDR);
		reply.dest_ip = arp_packet->src_ip;
		
		send_arp_packet(sockfd, reply.dest_mac, &reply);
	}

	
}



int process_AREP(int sockfd,unsigned char* src_mac,int from_index,t_arp* arp_packet)
{
	/*
		1. check if UID matches, drop otherwise.
		2. if cache miss, drop.
		3. update the cache hit- incomplete entry
		4. notify over domainfd.
	*/
	cache_entry * entry = NULL;
	struct hwaddr retaddr;
	int connfd;

	memset(&retaddr,0,sizeof(retaddr));
	
	if(arp_packet->proto_id!=ARP_9512K)
	{
		printf("somebody with same proto field trying to mess up X-( dropping..\n");
		return -1;
	}	

	entry = search_arp(arp_packet->src_ip);

	if(entry)
	{
		update_cache_entry(entry,arp_packet->src_ip,eth0_index,arp_packet->src_mac,arp_packet->hatype,-1);
		retaddr.sll_ifindex = entry->sll_ifindex;
		retaddr.sll_hatype = entry->sll_hatype;
		retaddr.sll_halen = IF_HADDR;
		connfd = entry->connfd;
		memcpy(retaddr.sll_addr,entry->mac,IF_HADDR);

		if(write(connfd,&retaddr,sizeof(retaddr))<=0)
		{
			perror("write error:");

		}
		close(connfd);		
	}
	else{
		printf("AREP dropped as no client is active..\n");
	}
}


int process_app_con(int sockfd,int connfd)
{
	fd_set rset;
	unsigned char *buff= malloc(1024);
	FD_ZERO(&rset);
	int len=0;
	unsigned long request_ip=0;
	struct hwaddr retaddr;
	t_arp request;
	int maxfdp=0;
	cache_entry * entry=NULL;
	

	memset(&retaddr,0,sizeof(retaddr));

	memset(&request,0,sizeof(request));


                FD_ZERO(&rset);
	//	while(1){
	
	                FD_SET(sockfd,&rset);
        	        FD_SET(connfd,&rset);

                	if(sockfd>connfd)
                	        maxfdp=sockfd+1;
               		else
                        	maxfdp=connfd+1;

	                select(maxfdp,&rset,NULL,NULL,NULL);
        	        if(FD_ISSET(sockfd,&rset))
  	                 {
        	                recv_process_pf_packet(sockfd);
       		         }
       		         if(FD_ISSET(connfd,&rset))
                	{
                        	len= read(connfd,buff,sizeof(buff));
				if(len<=0)
				{
					perror("proc appcon 2ndselect read: ");
					close(connfd);
					//break;
				}

			//	printf("read %d bytes",len);
				request_ip = *(unsigned long *)(buff);
			//	printf("%lx",request_ip);

				/*see if request ip is in cache*/
				entry=search_arp(request_ip);
				if(entry!=NULL)
				{
					//assert(entry->incomplete==0);
					retaddr.sll_ifindex = entry->sll_ifindex;
					retaddr.sll_hatype = entry->sll_hatype;
					retaddr.sll_halen = IF_HADDR;
					memcpy(retaddr.sll_addr,entry->mac,IF_HADDR);
	
					if(write(connfd,&retaddr,sizeof(retaddr))<=0)
					{
						perror("write error:");
				
					}
					close(connfd);					 
					/*write to connfd,as cache hit*/
					
					}
				else{	
					/*add incomplete entry to cache*/
					/*send AREQ broadcast and monitor connfd for fin and sockfd for AREP*/
					add_cache_entry(request_ip,eth0_index,NULL,1,connfd);
				
					request.type = AREQ;
					request.proto_id = ARP_9512K;
					request.hatype = 1;
					memcpy(request.src_mac,eth0_mac,IF_HADDR);
					request.src_ip = eth0_ip.sin_addr.s_addr;
					memset(request.dest_mac,0xff,IF_HADDR);
					request.dest_ip = request_ip;

					send_arp_packet(sockfd,request.dest_mac,&request);
	
					FD_ZERO(&rset);
        	        		FD_SET(sockfd,&rset);
                			FD_SET(connfd,&rset);
		               		if(sockfd>connfd)
        		          	      maxfdp=sockfd+1;
                			else
                        			maxfdp=connfd+1;

			                select(maxfdp,&rset,NULL,NULL,NULL);
		        	        
	
        		       		if(FD_ISSET(connfd,&rset))
		               		{
						/*monitoring for FIN, no data expected*/
						len=read(connfd,buff,sizeof(buff));
						if(len<=0)
						{
							printf("FIN on connfd..");
							delete_cache_entry(request_ip);
							//break;
						}		
					
					}
					if(FD_ISSET(sockfd,&rset))
	        		        {/*drop AREP if no entry found.*/
	                		        recv_process_pf_packet(sockfd);
	               			}
					
									
				
			}
                }
//	} commenting while for awhile.
}


int add_cache_entry(unsigned long ip, int if_index,unsigned char *mac,unsigned short sll_hatype,int connfd)
{
	cache_entry * entry = malloc(sizeof(cache_entry));
	memset(entry,0,sizeof(cache_entry));


	if(mac==NULL)
	entry->incomplete=1;

	entry->ip = ip;
	entry->sll_ifindex = if_index;
	entry->sll_hatype = sll_hatype;
	entry->connfd= connfd;

	if(mac)
	{
		memcpy(entry->mac,mac,IF_HADDR);
	}

	entry->next= cache_head;
	cache_head= entry;
	
	printf("added ip: %s\n",(char *)inet_ntoa(*(struct in_addr*)&ip));
	return 0;

}


int update_cache_entry(cache_entry *entry,unsigned long ip, int if_index,unsigned char *mac,unsigned short sll_hatype,int connfd)
{

	if(entry->ip!=ip)
	{
		printf("Bad entry passed to %s",__FUNCTION__);
		return -1;
	}
	else {

		entry->sll_ifindex = if_index;
		memcpy(entry->mac,mac,IF_HADDR);
		entry->sll_hatype = sll_hatype;
//		entry->connfd = connfd;
		entry->incomplete=0;
	}

	printf("updated ip: %s\n",(char*)inet_ntoa(*(struct in_addr*)&ip));
	return 0;
}

cache_entry * search_arp(unsigned long request_ip)
{
	cache_entry *node = cache_head;

	while(node)
	{
		if(node->ip==request_ip)
			break;
		else node=node->next;
		
	}
	
	return node;

}

int delete_cache_entry(unsigned long delete_ip)
{
	cache_entry * node= cache_head;
	cache_entry *prev=NULL;

	while(node)
	{
		if(node->ip==delete_ip)
			break;
		prev=node;
		node=node->next;	
	}

	if(node==NULL)
		return -1;

	if(prev==NULL)
	{
		/*head to be deleted*/
		cache_head= cache_head->next;
		free(node);
	}
	else {
		prev->next = node->next;
		free(node);	
	}
	printf("deleted ip: %s\n",(char*)inet_ntoa(*(struct in_addr*)&delete_ip));	
	return 0;
}
