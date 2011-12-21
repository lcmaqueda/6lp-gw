/*
 * This file implements the ethernet driver needed for the 6LP-GW
 */
#include "dev/eth_driver.h"
/*
 * We include the "contiki-net.h" file to get all the network functions.
 */
#include "contiki-net.h"

/*
 * And the "enc28j60.h" to get all the lower level driver functions.
 */
#include "dev/enc28j60.h"

/* 
 * We include as well "leds.h" to signal the hardware failure event.
 */
#include "dev/leds_hogaza.h"

/*
 * And "pgw_fwd.h", which holds the interface data structures.
 */
#include "net/p-gw/pgw_fwd.h"

/* The driver state */
static eth_driver_state_t eth_state = ETH_DRIVER_OFF;

static void init(void);
static void send(const void *payload, unsigned short payload_len);
static int read(const void *payload, unsigned short payload_len);
static int pending_packet(void);
static void on(void);
static void on(void);


/*---------------------------------------------------------------------------*/
/*
 * We declare the process that we use to register with the TCP/IP stack,
 * and to check for incoming packets.
 */
PROCESS(eth_driver_process, "eth_driver_process");
/*---------------------------------------------------------------------------*/
/*
 * This is the poll handler function in the process below. This poll handler
 * function checks for incoming packets and forwards them to the right 
 * interface or delivers them to the TCP/IP stack.
 */
static void
pollhandler(void)
{
	if (pending_packet()) {
		/* Set current incoming interface */
		incoming_if = IEEE_802_3;
		/* Read packet */
		uip_len = read(uip_buf, UIP_BUFSIZE);
		/* 
   	 * Forward the packet to the upper level in the stack
   	 */
  	NETSTACK_MAC_ETH.input();
	}
	/*
   * Now we'll make sure that the poll handler is executed repeatedly.
   * We do this by calling process_poll() with this process as its
   * argument.
   */
  process_poll(&eth_driver_process);
}
/*---------------------------------------------------------------------------*/
/*
 * Finally, we define the process that does the work. 
 */
PROCESS_THREAD(eth_driver_process, ev, data)
{
	/*
	 * This process has a poll handler, so we declare it here. Note that
	 * the PROCESS_POLLHANDLER() macro must come before the PROCESS_BEGIN()
   * macro.
   */
	PROCESS_POLLHANDLER(pollhandler());
	
	/*
	 * This process has an exit handler, so we declare it here. Note that
   * the PROCESS_EXITHANDLER() macro must come before the PROCESS_BEGIN()
   * macro.
   */
	PROCESS_EXITHANDLER(_nop());

  /*
   * The process begins here.
   */
  PROCESS_BEGIN();

  /*
   * Now we'll make sure that the poll handler is executed initially. We do
   * this by calling process_poll() with this process as its argument.
   */
  process_poll(&eth_driver_process);

  /*
   * And we wait for the process to exit.
   */
	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT);

  /*
   * Here ends the process.
   */
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
static void 
init() {
	enc28j60_init();
	on();
	process_start(&eth_driver_process, NULL);
}

/*---------------------------------------------------------------------------*/
static void
send(const void *payload, unsigned short payload_len)
{
	if (eth_state == ETH_DRIVER_ON) {
		enc28j60PacketSend(payload_len, (unsigned char*)payload);
	}
}

static int
read(const void *payload, unsigned short payload_len)
{
	if (eth_state == ETH_DRIVER_ON) {
		/* substract the 4-byte CRC and Link-layer header lengths */
		return enc28j60PacketReceive(payload_len, (unsigned char*)payload) - 4;
	} else {
		return 0;
	}
}

static int
pending_packet()
{
	if (eth_state == ETH_DRIVER_ON) {
		return enc28j60_pending_packet();
	} else {
		return 0;
	}
}

static void
on()
{
	eth_state = ETH_DRIVER_ON;
}

static void
off()
{
	eth_state = ETH_DRIVER_OFF;
}


const eth_driver_t eth_driver =
  {
  	init,
  	send,
  	read,
  	pending_packet,
  	on,
  	off
  };

/*---------------------------------------------------------------------------*/
