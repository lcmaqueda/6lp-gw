/*
 * This file implements the radio driver needed for the 6LP-GW
 */
#include "dev/radio_driver.h"
/*
 * We include the "contiki-net.h" file to get all the network functions.
 */
#include "contiki-net.h"
/*
 * And "cc2520ll.h" file to get all the lower level driver functions.
 */
#include "dev/cc2520ll.h"

/*
 * And "pgw_fwd.h", which holds the interface data structures.
 */
#include "net/p-gw/pgw_fwd.h"

static int init(void);
static int send(const void *payload, unsigned short payload_len);
static int read(void *buf, unsigned short buf_len);
static int pending_packet(void);
static int on(void);
static int off(void);

/* The driver state */
static radio_driver_state_t radio_state = OFF;
/*---------------------------------------------------------------------------*/
/*
 * We declare the process that we use to register with the TCP/IP stack,
 * and to check for incoming packets.
 */
PROCESS(radio_driver_process, "radio_driver_process");
/*---------------------------------------------------------------------------*/
/*
 * This is the poll handler function in the process below. This poll handler
 * function checks for incoming packets and forwards them to the right 
 * interface or delivers them to the TCP/IP stack.
 */
static void
pollhandler(void)
{
	if (cc2520ll_pending_packet()) {
		
		incoming_if = IEEE_802_15_4;
	
		packetbuf_clear();
    packetbuf_set_datalen(read(packetbuf_dataptr(), PACKETBUF_SIZE));
		/* 
	   * Forward the packet to the upper level in the stack
	   */
  	NESTACK_MAC_RADIO.input();
	}
  /*
   * Now we'll make sure that the poll handler is executed repeatedly.
   * We do this by calling process_poll() with this process as its
   * argument.
   */
  process_poll(&radio_driver_process);
}
/*---------------------------------------------------------------------------*/
/*
 * Finally, we define the process that does the work. 
 */
PROCESS_THREAD(radio_driver_process, ev, data)
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
  process_poll(&radio_driver_process);

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
static int 
init()
{
	if (cc2520ll_init() == FAILED) {
		return 0;
	} else {
		on();
		process_start(&radio_driver_process, NULL);
		return 1;
	}
}

/*---------------------------------------------------------------------------*/
static int
send(const void *payload, unsigned short payload_len)
{
	if (radio_state == ON) {
		return cc2520ll_packetSend(payload, payload_len);
	}
	return 0;
}

static int 
read(void *buf, unsigned short buf_len) 
{
	if (radio_state == ON) {
		/* substract CRC length */
		return cc2520ll_packetReceive(buf, buf_len) - 2;
	} else {
		return 0;
	}
}

static int
pending_packet()
{
	if (radio_state == ON) {
		return cc2520ll_pending_packet();
	} else {
		return 0;
	}
}

static int
on()
{
	radio_state = ON;
	return 1;
}

static int
off()
{
	radio_state = OFF;
	return 1;
}

/* These functions are not used; They are defined only for compliance with 
 * struct radio_driver defined in radio.h*/
int 
prepare (const void *payload, unsigned short payload_len)
{return 1;}

int
transmit(unsigned short transmit_len)
{return 1;}

int
channel_clear(void)
{return 1;}
int
receiving_packet(void)
{return 0;}

const struct radio_driver radio_driver =
  {
  	init,
  	prepare,
  	transmit,
    send,
		read,
		channel_clear,
		receiving_packet,
		pending_packet,
		on,
		off
  };

/*---------------------------------------------------------------------------*/
