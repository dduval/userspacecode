/* 	
	Netlink example that uses routing table operations to get
	feedback from the kernel. Error handling code was skipped 
	in order to reduce code length.

	Copyright Dominic Duval <dduval@redhat.com> according to the terms
        of the GNU Public License.

*/

#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define MAX_PAYLOAD 1024

int read_event(int sock)
{
        int ret;
	struct nlmsghdr *nlh;

	nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	ret = recv(sock, (void *) nlh,  NLMSG_SPACE(MAX_PAYLOAD), 0);

	switch (nlh->nlmsg_type) {
		case RTM_NEWROUTE:
			printf("NEWROUTE\n");
			break;
		case RTM_DELROUTE:
			printf("DELROUTE\n");
			break;
		case RTM_GETROUTE:
			printf("GETROUTE\n");
			break;
		default:
			printf("Unknown type\n");
			break;
	}
        return 0;
}

int main(int argc, char *argv[])
{       
	int nls = socket(AF_NETLINK,SOCK_RAW,NETLINK_ROUTE);
        struct sockaddr_nl addr;

        memset((void *)&addr, 0, sizeof(addr));

        addr.nl_family = AF_NETLINK;
        addr.nl_pid = getpid();
        addr.nl_groups = RTMGRP_IPV4_ROUTE;
        bind(nls,(struct sockaddr *)&addr,sizeof(addr));

        while(1)
		read_event(nls);
        return 0;
}

