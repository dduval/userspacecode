/* 	
	Netlink example that receives feedback from the kernel. 
	Error handling was skipped in order to reduce code length.

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
#define NL_EXAMPLE 19
#define NL_GROUP 1

int read_event(int sock)
{
	int ret;
	struct nlmsghdr *nlh;

	nlh = (struct nlmsghdr *) malloc(NLMSG_LENGTH(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_LENGTH(MAX_PAYLOAD));
	ret = recv(sock, (void *) nlh, NLMSG_LENGTH(MAX_PAYLOAD), 0);

	printf("Message size: %d , Message: %s\n", ret, NLMSG_DATA(nlh));

	return 0;
}

int main(int argc, char *argv[])
{
	struct sockaddr_nl addr;
	int nls;


	/* Set up the netlink socket */
	nls = socket(AF_NETLINK, SOCK_RAW, NL_EXAMPLE);

	memset((void *) &addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = NL_GROUP;
	bind(nls, (struct sockaddr *) &addr, sizeof(addr));

	while (1)
		read_event(nls);

	return 0;
}
