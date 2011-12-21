/**
 * \file		dhcp-client.c
 *
 * \brief		DHCP client Contiki process
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#include "contiki-net.h"
#include "net/uipv4/uipv4.h"
#include "net/uipv4/dhcpc.h"

PROCESS(dhcp_process, "DHCP client");

/*-----------------------------------------------------------------------------------*/
PROCESS_THREAD(dhcp_process, ev, data)
{
  PROCESS_BEGIN();
  
  dhcpc_init(uip_ethaddr.addr, sizeof(uip_ethaddr.addr));
	dhcpc_request();

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpipv4_event || ev == PROCESS_EVENT_TIMER) {
      dhcpc_appcall(ev, data);
    }    
  }

  PROCESS_END();
}

/*-----------------------------------------------------------------------------------*/
void
dhcpc_configured(const struct dhcpc_state *s)
{
  uipv4_sethostaddr(&s->ipaddr);
  uipv4_setnetmask(&s->netmask);
  uipv4_setdraddr(&s->default_router);
}

/*-----------------------------------------------------------------------------------*/
void
dhcpc_unconfigured(const struct dhcpc_state *s)
{
  uipv4_sethostaddr(&uipv4_all_zeroes_addr);
  uipv4_setnetmask(&uipv4_all_zeroes_addr);
  uipv4_setdraddr(&uipv4_all_zeroes_addr);
}
/*-----------------------------------------------------------------------------------*/
