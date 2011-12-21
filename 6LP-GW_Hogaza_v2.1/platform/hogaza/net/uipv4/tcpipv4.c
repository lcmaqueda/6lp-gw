/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 *
 * $Id: tcpipv6.c,v 1.30 2010/10/29 05:36:07 adamdunkels Exp $
 */
/**
 * \file
 *         Code for tunnelling uIP packets over the Rime mesh routing module
 *
 * \author  Adam Dunkels <adam@sics.se>\author
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 */
#include "contiki-net.h"
#include "net/uipv4/uipv4.h"
#include "net/uipv4/tcpipv4.h"
#include "net/uip-packetqueue.h"

#include <string.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#endif

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#define UIP_ICMP_BUF ((struct uip_icmp_hdr *)&uip_buf[UIPV4_LLIPH_LEN + uip_ext_len])
#define UIPV4_IP_BUF ((struct uipv4_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIPV4_TCP_BUF ((struct uipv4_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

process_event_t tcpipv4_event;


/*periodic check of active connections*/
static struct etimer periodic;

/**
 * \internal Structure for holding a TCP port and a process ID.
 */
struct listenport {
  u16_t port;
  struct process *p;
};

static struct internal_state {
  struct listenport listenports[UIPV4_LISTENPORTS];
  struct process *p;
} s;

enum {
  TCP_POLL,
  UDP_POLL,
  PACKET_INPUT
};

u8_t
tcpipv4_output(void)
{
  NETSTACK_MAC_ETH.send();
  uip_len = 0;
  uip_ext_len = 0;
  return 0;
}

PROCESS(tcpipv4_process, "TCP/IP stack");

/*---------------------------------------------------------------------------*/
static void
start_periodic_tcp_timer(void)
{
  if(etimer_expired(&periodic)) {
    etimer_restart(&periodic);
  }
}

/*---------------------------------------------------------------------------*/
static void
check_for_tcp_syn(void)
{
  /* This is a hack that is needed to start the periodic TCP timer if
     an incoming packet contains a SYN: since uIP does not inform the
     application if a SYN arrives, we have no other way of starting
     this timer.  This function is called for every incoming IP packet
     to check for such SYNs. */
#define TCP_SYN 0x02
  if(UIPV4_IP_BUF->proto == UIP_PROTO_TCP &&
     (UIPV4_TCP_BUF->flags & TCP_SYN) == TCP_SYN) {
    start_periodic_tcp_timer();
  }
}
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  if(uip_len > 0) {
    check_for_tcp_syn();
    uipv4_input();
    if(uip_len > 0) {
      PRINTF("tcpip packet_input output len %d\n", uip_len);
      tcpipv4_output();
    }
  }
}
/*---------------------------------------------------------------------------*/
#if UIPV4_TCP
#if UIP_ACTIVE_OPEN
struct uipv4_conn *
tcp4_connect(uip_ip4addr_t *ripaddr, u16_t port, void *appstate)
{
  struct uipv4_conn *c;
  
  c = uipv4_connect(ripaddr, port);
  if(c == NULL) {
    return NULL;
  }

  c->appstate.p = PROCESS_CURRENT();
  c->appstate.state = appstate;
  
  tcpipv4_poll_tcp(c);
  
  return c;
}
#endif /* UIP_ACTIVE_OPEN */
/*---------------------------------------------------------------------------*/
void
tcp4_unlisten(u16_t port)
{
  static unsigned char i;
  struct listenport *l;

  l = s.listenports;
  for(i = 0; i < UIPV4_LISTENPORTS; ++i) {
    if(l->port == port &&
       l->p == PROCESS_CURRENT()) {
      l->port = 0;
      uipv4_unlisten(port);
      break;
    }
    ++l;
  }
}
/*---------------------------------------------------------------------------*/
void
tcp4_listen(u16_t port)
{
  static unsigned char i;
  struct listenport *l;

  l = s.listenports;
  for(i = 0; i < UIPV4_LISTENPORTS; ++i) {
    if(l->port == 0) {
      l->port = port;
      l->p = PROCESS_CURRENT();
      uipv4_listen(port);
      break;
    }
    ++l;
  }
}
/*---------------------------------------------------------------------------*/
void
tcp4_attach(struct uipv4_conn *conn,
	   void *appstate)
{
  register uip_tcp_appstate_t *s;

  s = &conn->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;
}

#endif /* UIPV4_TCP */
/*---------------------------------------------------------------------------*/
#if UIPV4_UDP
void
udp4_attach(struct uipv4_udp_conn *conn,
	   void *appstate)
{
  register uip_udp_appstate_t *s;

  s = &conn->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;
}
/*---------------------------------------------------------------------------*/
struct uipv4_udp_conn *
udp4_new(const uip_ip4addr_t *ripaddr, u16_t port, void *appstate)
{
  struct uipv4_udp_conn *c;
  uip_udp_appstate_t *s;
  
  c = uipv4_udp_new(ripaddr, port);
  if(c == NULL) {
    return NULL;
  }

  s = &c->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;

  return c;
}
/*---------------------------------------------------------------------------*/
struct uipv4_udp_conn *
udp4_broadcast_new(u16_t port, void *appstate)
{
  uip_ip4addr_t addr;
  struct uipv4_udp_conn *conn;

  uip_ip4addr(&addr, 255,255,255,255);
  conn = udp4_new(&addr, port, appstate);
  if(conn != NULL) {
    udp4_bind(conn, port);
  }
  return conn;
}
#endif /* UIPV4_UDP */
/*---------------------------------------------------------------------------*/
static void
eventhandler(process_event_t ev, process_data_t data)
{
#if UIPV4_TCP
  static unsigned char i;
  register struct listenport *l;
#endif /*UIPV4_TCP*/
  struct process *p;
   
  switch(ev) {
    case PROCESS_EVENT_EXITED:
      /* This is the event we get if a process has exited. We go through
         the TCP/IP tables to see if this process had any open
         connections or listening TCP ports. If so, we'll close those
         connections. */

      p = (struct process *)data;
#if UIPV4_TCP
      l = s.listenports;
      for(i = 0; i < UIPV4_LISTENPORTS; ++i) {
        if(l->p == p) {
          uipv4_unlisten(l->port);
          l->port = 0;
          l->p = PROCESS_NONE;
        }
        ++l;
      }
	 
      {
        register struct uipv4_conn *cptr;
	    
        for(cptr = &uipv4_conns[0]; cptr < &uipv4_conns[UIPV4_CONNS]; ++cptr) {
          if(cptr->appstate.p == p) {
            cptr->appstate.p = PROCESS_NONE;
            cptr->tcpstateflags = UIP_CLOSED;
          }
	       
        }
	    
      }
#endif /* UIPV4_TCP */
#if UIPV4_UDP
      {
        register struct uipv4_udp_conn *cptr;
        for(cptr = &uipv4_udp_conns[0];
            cptr < &uipv4_udp_conns[UIPV4_UDP_CONNS]; ++cptr) {
          if(cptr->appstate.p == p) {
            cptr->lport = 0;
          }
        }
      
      }
#endif /* UIPV4_UDP */
      break;

    case PROCESS_EVENT_TIMER:
      /* We get this event if one of our timers have expired. */
      {
        /* Check the clock so see if we should call the periodic uIP
           processing. */
        if(data == &periodic &&
           etimer_expired(&periodic)) {
#if UIPV4_TCP
          for(i = 0; i < UIPV4_CONNS; ++i) {
            if(uipv4_conn_active(i)) {
              /* Only restart the timer if there are active
                 connections. */
              etimer_restart(&periodic);
              uipv4_periodic(i);
              if(uip_len > 0) {
		PRINTF("tcpipv4_output from periodic len %d\n", uip_len);
                tcpipv4_output();
		PRINTF("tcpipv4_output after periodic len %d\n", uip_len);
              }
            }
          }
#endif /* UIPV4_TCP */
        }
      }
      break;
	 
#if UIPV4_TCP
    case TCP_POLL:
      if(data != NULL) {
        uipv4_poll_conn(data);
        if(uip_len > 0) {
	  PRINTF("tcpipv4_output from tcp poll len %d\n", uip_len);
          tcpipv4_output();
        }
        /* Start the periodic polling, if it isn't already active. */
        start_periodic_tcp_timer();
      }
      break;
#endif /* UIPV4_TCP */
#if UIPV4_UDP
    case UDP_POLL:
      if(data != NULL) {
        uipv4_udp_periodic_conn(data);
        if(uip_len > 0) {
          tcpipv4_output();
        }
      }
      break;
#endif /* UIPV4_UDP */

    case PACKET_INPUT:
      packet_input();
      break;
  };
}
/*---------------------------------------------------------------------------*/
void
tcpipv4_input(void)
{
  process_post_synch(&tcpipv4_process, PACKET_INPUT, NULL);
  uip_len = 0;
}
/*---------------------------------------------------------------------------*/
#if UIPV4_UDP
void
tcpipv4_poll_udp(struct uipv4_udp_conn *conn)
{
  process_post(&tcpipv4_process, UDP_POLL, conn);
}
#endif /* UIPV4_UDP */
/*---------------------------------------------------------------------------*/
#if UIPV4_TCP
void
tcpipv4_poll_tcp(struct uipv4_conn *conn)
{
  process_post(&tcpipv4_process, TCP_POLL, conn);
}
#endif /* UIPV4_TCP */
/*---------------------------------------------------------------------------*/
void
tcpipv4_uipcall(void)
{
  register uip_udp_appstate_t *ts;
  
#if UIPV4_UDP
  if(uipv4_conn != NULL) {
    ts = &uipv4_conn->appstate;
  } else {
    ts = &uipv4_udp_conn->appstate;
  }
#else /* UIPV4_UDP */
  ts = &uipv4_conn->appstate;
#endif /* UIPV4_UDP */

#if UIPV4_TCP
 	{
   	static unsigned char i;
   	register struct listenport *l;
   
   	/* If this is a connection request for a listening port, we must
    	 mark the connection with the right process ID. */
   	if(uipv4_connected()) {
    	l = &s.listenports[0];
     	for(i = 0; i < UIPV4_LISTENPORTS; ++i) {
      	if(l->port == uipv4_conn->lport &&
	  				l->p != PROCESS_NONE) {
	 				ts->p = l->p;
	 				ts->state = NULL;
	 				break;
       	}
       	++l;
     	}
     
     	/* Start the periodic polling, if it isn't already active. */
     	start_periodic_tcp_timer();
   	}
 	}
#endif /* UIPV4_TCP */
  
  if(ts->p != NULL) {
    process_post_synch(ts->p, tcpipv4_event, ts->state);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcpipv4_process, ev, data)
{
  PROCESS_BEGIN();
  
#if UIPV4_TCP
 	{
  	static unsigned char i;
   
   	for(i = 0; i < UIPV4_LISTENPORTS; ++i) {
    	s.listenports[i].port = 0;
   	}
   	s.p = PROCESS_CURRENT();
 	}
#endif

  tcpipv4_event = process_alloc_event();
  etimer_set(&periodic, CLOCK_SECOND / 2);

  uipv4_init();

  while(1) {
    PROCESS_YIELD();
    eventhandler(ev, data);
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void tcpipv4_init() {
	process_start(&tcpipv4_process, NULL);
}

const struct network_ipv4_driver ipv4_driver = {
  "network_ipv4_driver",
  tcpipv4_init,
  tcpipv4_input,
  tcpipv4_output
};

/*---------------------------------------------------------------------------*/
