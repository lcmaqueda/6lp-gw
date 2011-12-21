/**
 * \file		contiki-hogaza-6lp-gw-main.c
 *
 * \brief		6LP-GW Application's main file.
 *
 * \author		Luis Maqueda <luis@sen.se>
 */


#include <msp430f5435a.h>

#include "contiki.h"
#include "sys/process.h"
#include "sys/procinit.h"
#include "contiki-net.h"
#include "dev/leds_hogaza.h"
#include "dev/msp430_arch.h"
#include "clock_arch.h"
#include "net/p-gw/pgw.h"
#include "net/uipv4/uipv4.h"
#include "dhcpc/dhcp-client.h"

#define 	WAFERID			0x01A0A
#define 	WAFERIPOSX		0x01A0E
#define 	WAFERIPOSY		0x01A10

/*---------------------------------------------------------------------------*/
#pragma FUNC_NEVER_RETURNS(main);
void
main(void)
{

  msp430_init();
  clock_init();

  /* initialize uip variables */
  memset(uip_buf, 0, UIP_CONF_BUFFER_SIZE);
  uip_len = 0;

  leds_init();
  buttons_init();

  /* Create MAC addresses based on a hash of the unique id (wafer id + x-pos
   * in wafer + y-pos in wafer). Note that despite all, this MAC address are
   * not necessarily unique.
   * We use rime addresses in order to use useful rime address handling
   * functions.
   * The sicslowmac layer requires the MAC address to be placed in the global
   * variable rimeaddr_node_addr (declared in rimeaddr.h). */
  rimeaddr_node_addr.u8[0] = NODE_BASE_ADDR0;
  rimeaddr_node_addr.u8[1] = NODE_BASE_ADDR1;
  rimeaddr_node_addr.u8[2] = NODE_BASE_ADDR2;
  rimeaddr_node_addr.u8[3] = NODE_BASE_ADDR3;
  rimeaddr_node_addr.u8[4] = NODE_BASE_ADDR4;
  /* lowest byte of wafer id */
  rimeaddr_node_addr.u8[5] = *((unsigned char*)(WAFERID+2));
  /* lowest byte of x-pos in wafer */
  rimeaddr_node_addr.u8[6] = *((unsigned char*)(WAFERIPOSX));
  /* lowest byte of y-pos in wafer */
  rimeaddr_node_addr.u8[7] = *((unsigned char*)(WAFERIPOSY));

  /* The following line sets the uIP's link-layer address. This must be done
   * before the tcpip_process is started since in its initialization
   * routine the function uip_netif_init() will be called from inside
   * uip_init()and there the default IPv6 address will be set by combining
   * the link local prefix (fe80::/64)and the link layer address. */
  rimeaddr_copy((rimeaddr_t*)&uip_lladdr.addr, &rimeaddr_node_addr);

  /* For the IPv4 stack we need an Ethernet address in uip_ethaddr */
  uip_ethaddr.addr[0] = NODE_BASE_ADDR0;
  uip_ethaddr.addr[1] = NODE_BASE_ADDR1;
  uip_ethaddr.addr[2] = NODE_BASE_ADDR2;
  uip_ethaddr.addr[3] = *((unsigned char*)(WAFERID+2));
  uip_ethaddr.addr[4] = *((unsigned char*)(WAFERIPOSX));
  uip_ethaddr.addr[5] = *((unsigned char*)(WAFERIPOSY));

  /* Initialize the process module */
  process_init();
  /* etimers must be started before ctimer_init */
  process_start(&etimer_process, NULL);

  /* Initialize stack protocols */
  pgw_netstack_init();

  /* Initialice the DHCP client */
  process_start(&dhcp_process, NULL);

  /* Enter main loop */
  while(1) {
	/* poll every running process which has requested to be polled */
	process_run();
	/* etimer_request_poll() called in timer interrupt routine */
  }
}
