#include "arp.h"

cache_entry *cache_head=NULL;


int send_arp_packet(int sockfd,unsigned char *dest_mac, t_arp *arp_packet)
{
        /*target address*/
        struct sockaddr_ll socket_dest_address;
        char* src_mac=NULL;
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
        printf("ARP at node %s:",get_name(eth0_ip));

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


int recv_process_pf_packet(int sockfd,int domainfd)
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
                        process_AREQ(sockfd,domainfd,src_mac,from.sll_ifindex,arp_packet);

                break;
                case AREP:

                        dprintf("RREP from %x:%x:%x:%x:%x:%x to me at %x:%x:%x:%x:%x:%x \n",
                                src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5],
                              dest_mac[0],dest_mac[1],dest_mac[2],dest_mac[3],dest_mac[4],dest_mac[5]);
                        process_AREP(sockfd,domainfd,src_mac,from.sll_ifindex,arp_packet);
                break;
                default:
                        printf("Garbage packet on ARP.. dropping\n");
                break;
        }



//      printf("data recieved: %s",(char*)recv_buffer+14);

        free(recv_buffer);
        return 0;

}

int  process_AREQ(int sockfd,int domainfd,unsigned char *src_mac,int from_index,t_arp* arp_packet)
{
	/*
		1. check if UID matches, drop otherwise
		2. see if packet is meant for you, reply or drop accordingly.
		3. Check the source ip, update the entry if it already exists.	
			
	*/

}



int process_AREP(int sockfd,int domainfd,unsigned char* src_mac,int from_index,t_arp* arp_packet)
{
	/*
		1. check if UID matches, drop otherwise.
		2. if cache miss, drop.
		3. update the cache hit- incomplete entry
		4. notify over domainfd.
	*/

}


int process_app_conn(int sockfd,int connfd)
{
	fd_set rset;
	unsigned char *buff= malloc(1024);
	FD_ZERO(&rset);
	int len=0;
	unsigned long request_ip=0;
	struct hwaddr retaddr;
	t_arp request;

	memset(&retaddr,0,sizeof(retaddr));

	memset(&request,0,sizeof(request));


                FD_ZERO(&rset);
                FD_SET(sockfd,&rset);
                FD_SET(connfd,&rset);
                if(sockfd>connfd)
                        maxfdp=sockfd+1;
                else
                        maxfdp=connfd+1;



                select(maxfdp,&rset,NULL,NULL,NULL);
                if(FD_ISSET(sockfd,&rset))
                {
                        recv_process_pf_packet(sockfd,connfd);
                }
                if(FD_ISSET(connfd,&rset))
                {
                        len= read(connfd,buff,sizeof(buff));
			if(len<=0)
			{
				perror("read: ");
			}
			request_ip = *(unsigned long *)(buff);
			/*see if request ip is in cache*/
			entry=search_arp(request_ip);
			if(entry!=NULL)
			{
				assert(entry->incomplete==0);
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
				add_cache_entry(request_ip,eth0_ifindex,NULL,1,connfd);
				
				request.type = AREQ;
				request.proto_id = ARP_9512K;
				request.hatype = 1;
				memcpy(request.src_mac,eth0_mac,IF_HADDR);
				request.src_ip = eth0_ip;
				memset(request.dest_mac,0xff,IF_HADDR);
				request.dest_ip = request_ip;

				send_arp_packet(sockfd,request.dest_mac,&request);

	`			FD_ZERO(&rset);
        	        	FD_SET(sockfd,&rset);
                		FD_SET(connfd,&rset);
	               		if(sockfd>connfd)
        	          	      maxfdp=sockfd+1;
                		else
                        		maxfdp=connfd+1;



		                select(maxfdp,&rset,NULL,NULL,NULL);
	        	        

        	       		if(FD_ISSET(connfd,&rset))
	               		{
					len=read(connfd,buff,sizeof(buff));
					if(len<=0)
					{
						printf("FIN on connfd..");
						delete_cache_entry(request_ip);
					}
					
				}
				if(FD_ISSET(sockfd,&rset))
	        	        {/*drop AREP if no entry found.*/
	                	        recv_process_pf_packet(sockfd,connfd);
	               		}
					
									
				
			}
                }

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
	
	printf("added ip: %s\n",(char *)inet_ntoa(ip));
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
		entry->connfd = connfd;
		entry->incomplete=0;
	}

	printf("updated ip: %s\n",(char*)inet_ntoa(ip));
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
	printf("deleted ip: %s\n",(char*)inet_ntoa(delete_ip));	
	return 0;
}
