#include "net/pgw_netstack.h"

void
pgw_netstack_init(void)
{
	NETSTACK_ETHERNET.init();
	NETSTACK_MAC_ETH.init();

	NETSTACK_RADIO.init();
	NESTACK_MAC_RADIO.init();
	
	NETSTACK_6LOWPAN.init();

	NETSTACK_NETWORK_IPV4.init();
	NETSTACK_NETWORK_IPV6.init();

	NETSTACK_6LPGW.init();
}
