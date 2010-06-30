/* 	
	Netlink demo that sends a message from kernel space to user space.

	Copyright Dominic Duval <dduval@redhat.com> according to the terms
        of the GNU Public License.

*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>

#define MAX_PAYLOAD 1024
#define NL_EXAMPLE 19
#define NL_GROUP 1
#define NETLINK_MESSAGE "This message originated from the kernel!"

struct sock *nl_sk = NULL;


static int __init init_mod(void)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh;

	nl_sk = netlink_kernel_create(NL_EXAMPLE, NULL);

	if (nl_sk == 0)
		return -1;

	skb = alloc_skb(NLMSG_SPACE(MAX_PAYLOAD), GFP_KERNEL);
	nlh = (struct nlmsghdr *) skb_put(skb, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = 0;
	nlh->nlmsg_flags = 0;
	strcpy(NLMSG_DATA(nlh), NETLINK_MESSAGE);
	NETLINK_CB(skb).pid = 0;
	NETLINK_CB(skb).dst_pid = 0;
	NETLINK_CB(skb).dst_groups = NL_GROUP;

	netlink_broadcast(nl_sk, skb, 0, NL_GROUP, GFP_KERNEL);
	sock_release(nl_sk->sk_socket);

	return -1;		// Always remove module immediately after loading
}

module_init(init_mod);

MODULE_LICENSE("GPL");
