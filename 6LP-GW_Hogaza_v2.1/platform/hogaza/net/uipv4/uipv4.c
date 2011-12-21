#define DEBUG_PRINTF(...) /*printf(__VA_ARGS__)*/

/**
 * \addtogroup uip
 * @{
 */

/**
 * \file
 * The uIP TCP/IP stack code.
 * \author Adam Dunkels <adam@dunkels.com>
 */

/*
 * Copyright (c) 2001-2003, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: uip.c,v 1.30 2010/10/19 18:29:04 adamdunkels Exp $
 *
 */

/*
 * uIP is a small implementation of the IP, UDP and TCP protocols (as
 * well as some basic ICMP stuff). The implementation couples the IP,
 * UDP, TCP and the application layers very tightly. To keep the size
 * of the compiled code down, this code frequently uses the goto
 * statement. While it would be possible to break the uipv4_process()
 * function into many smaller functions, this would increase the code
 * size because of the overhead of parameter passing and the fact that
 * the optimier would not be as efficient.
 *
 * The principle is that we have a small buffer, called the uip_buf,
 * in which the device driver puts an incoming packet. The TCP/IP
 * stack parses the headers in the packet, and calls the
 * application. If the remote host has sent data to the application,
 * this data is present in the uip_buf and the application read the
 * data from there. It is up to the application to put this data into
 * a byte stream if needed. The application will not be fed with data
 * that is out of sequence.
 *
 * If the application whishes to send data to the peer, it should put
 * its data into the uip_buf. The uip_appdata pointer points to the
 * first available byte. The TCP/IP stack will calculate the
 * checksums, and fill in the necessary header fields and finally send
 * the packet back to the peer.
*/

#include "net/uipv4/uipv4.h"
#include "net/uip_common.h"
#include "net/uipv4/uipv4opt.h"
#include "net/uipv4/uipv4_arp.h"
#include "net/uip_arch.h"

#include <string.h>

/*---------------------------------------------------------------------------*/
/* Variable definitions. */


/* The IP address of this host. If it is defined to be fixed (by
   setting UIPV4_FIXEDADDR to 1 in uipopt.h), the address is set
   here. Otherwise, the address */
#if UIP_FIXEDADDR > 0
const uip_ip4addr_t uipv4_hostaddr =
  { UIP_IPADDR0, UIP_IPADDR1, UIP_IPADDR2, UIP_IPADDR3 };
const uip_ip4addr_t uipv4_draddr =
  { UIP_DRIPADDR0, UIP_DRIPADDR1, UIP_DRIPADDR2, UIP_DRIPADDR3 };
const uip_ip4addr_t uipv4_netmask =
  { UIP_NETMASK0, UIP_NETMASK1, UIP_NETMASK2, UIP_NETMASK3 };
#else
uip_ip4addr_t uipv4_hostaddr, uipv4_draddr, uipv4_netmask;
#endif /* UIPV4_FIXEDADDR */

const uip_ip4addr_t uipv4_broadcast_addr =
  { { 0xff, 0xff, 0xff, 0xff } };
const uip_ip4addr_t uipv4_all_zeroes_addr = { { 0x0, /* rest is 0 */ } };

#if UIPV4_FIXEDETHADDR
const struct uip_eth_addr uip_ethaddr = {{UIP_ETHADDR0,
					  UIP_ETHADDR1,
					  UIP_ETHADDR2,
					  UIP_ETHADDR3,
					  UIP_ETHADDR4,
					  UIP_ETHADDR5}};
#else
struct uip_eth_addr uip_ethaddr = {{0,0,0,0,0,0}};
#endif

void *uipv4_sappdata;              /* The uip_appdata pointer points to
				    the application data which is to
				    be sent. */

u16_t uipv4_slen;
                             /* The uip_len is either 8 or 16 bits,
				depending on the maximum packet
				size. */

u8_t uipv4_flags;     /* The uipv4_flags variable is used for
				communication between the TCP/IP stack
				and the application program. */
				
struct uipv4_conn *uipv4_conn;   /* uipv4_conn always points to the current
				IPv4 connection. */
#if UIPV4_UDP || UIPV4_TCP
static u8_t c;
#endif /* UIPV4_UDP || UIPV4_TCP */

#if UIPV4_TCP
struct uipv4_conn uipv4_conns[UIPV4_CONNS];
                             /* The uipv4_conns array holds all TCP
				connections. */
u16_t uipv4_listenports[UIPV4_LISTENPORTS];
                             /* The uipv4_listenports list all currently
				listening ports. */
#endif /* UIPV4_TCP */				

#if UIPV4_UDP
struct uipv4_udp_conn *uipv4_udp_conn;
struct uipv4_udp_conn uipv4_udp_conns[UIPV4_UDP_CONNS];
#endif /* UIPV4_UDP */

static u16_t ipid;           /* Ths ipid variable is an increasing
				number that is used for the IP ID
				field. */

void uip_setipid(u16_t id) { ipid = id; }


#if UIPV4_ACTIVE_OPEN || UIPV4_UDP
static u16_t lastport;       /* Keeps track of the last port used for
				a new connection. */
#endif /* UIPV4_ACTIVE_OPEN || UIPV4_UDP */

#if UIPV4_TCP
static u8_t iss[4];          /* The iss variable is used for the TCP
				initial sequence number. */

/* Temporary variables. */
static u8_t opt;
static u16_t tmp16;
#endif /* UIPV4_TCP */

/* Structures and definitions. */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20
#define TCP_CTL 0x3f

#define TCP_OPT_END     0   /* End of TCP options list */
#define TCP_OPT_NOOP    1   /* "No-operation" TCP option */
#define TCP_OPT_MSS     2   /* Maximum segment size TCP option */

#define TCP_OPT_MSS_LEN 4   /* Length of TCP MSS option. */

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO       8

#define ICMP_DEST_UNREACHABLE        3
#define ICMP_PORT_UNREACHABLE        3

/* Macros. */
#define BUF ((struct uipv4_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define FBUF ((struct uipv4_tcpip_hdr *)&uip_reassbuf[0])
#define ICMPBUF ((struct uipv4_icmpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UDPBUF ((struct uipv4_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#if UIP_STATISTICS == 1
struct uip_stats uip_stat;
#define UIP_STAT(s) s
#else
#define UIP_STAT(s)
#endif /* UIP_STATISTICS == 1 */

#if UIP_LOGGING == 1
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

#if ! UIP_ARCH_ADD32
void
uip_add32(u8_t *op32, u16_t op16)
{
  uip_acc32[3] = op32[3] + (op16 & 0xff);
  uip_acc32[2] = op32[2] + (op16 >> 8);
  uip_acc32[1] = op32[1];
  uip_acc32[0] = op32[0];
  
  if(uip_acc32[2] < (op16 >> 8)) {
    ++uip_acc32[1];
    if(uip_acc32[1] == 0) {
      ++uip_acc32[0];
    }
  }
  
  
  if(uip_acc32[3] < (op16 & 0xff)) {
    ++uip_acc32[2];
    if(uip_acc32[2] == 0) {
      ++uip_acc32[1];
      if(uip_acc32[1] == 0) {
				++uip_acc32[0];
      }
    }
  }
}

#endif /* UIP_ARCH_ADD32 */

#if ! UIP_ARCH_CHKSUM
/*---------------------------------------------------------------------------*/
static u16_t
chksum(u16_t sum, const u8_t *data, u16_t len)
{
  u16_t t;
  const u8_t *dataptr;
  const u8_t *last_byte;

  dataptr = data;
  last_byte = data + len - 1;
  
  while(dataptr < last_byte) {	/* At least two more bytes */
    t = (dataptr[0] << 8) + dataptr[1];
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
    dataptr += 2;
  }
  
  if(dataptr == last_byte) {
    t = (dataptr[0] << 8) + 0;
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
  }

  /* Return sum in host byte order. */
  return sum;
}
/*---------------------------------------------------------------------------*/
#ifndef UIP_ARCH_IPCHKSUM
u16_t
uipv4_ipchksum(void)
{
  u16_t sum;

  sum = chksum(0, &uip_buf[UIP_LLH_LEN], UIPV4_IPH_LEN);
  DEBUG_PRINTF("uip_ipchksum: sum 0x%04x\n", sum);
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
#endif
/*---------------------------------------------------------------------------*/
static u16_t
upper_layer_chksum(u8_t proto)
{
  u16_t upper_layer_len;
  u16_t sum;
  
  upper_layer_len = (((u16_t)(BUF->len[0]) << 8) + BUF->len[1]) - UIPV4_IPH_LEN;
  
  /* First sum pseudoheader. */
  
  /* IP protocol and length fields. This addition cannot carry. */
  sum = upper_layer_len + proto;
  /* Sum IP source and destination addresses. */
  sum = chksum(sum, (u8_t *)&BUF->srcipaddr, 2 * sizeof(uip_ip4addr_t));

  /* Sum TCP header and data. */
  sum = chksum(sum, &uip_buf[UIPV4_IPH_LEN + UIP_LLH_LEN],
	       upper_layer_len);
    
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/
u16_t
uipv4_udpchksum(void)
{
  return upper_layer_chksum(UIP_PROTO_UDP);
}
/*---------------------------------------------------------------------------*/
u16_t
uipv4_tcpchksum(void)
{
  return upper_layer_chksum(UIP_PROTO_TCP);
}
/*---------------------------------------------------------------------------*/
#endif /* UIP_ARCH_CHKSUM */
/*---------------------------------------------------------------------------*/
void
uipv4_init(void)
{
#if UIPV4_TCP	
  for(c = 0; c < UIPV4_LISTENPORTS; ++c) {
    uipv4_listenports[c] = 0;
  }
  for(c = 0; c < UIPV4_CONNS; ++c) {
    uipv4_conns[c].tcpstateflags = UIP_CLOSED;
  }
#endif /* UIPV4_TCP */  

#if UIPV4_ACTIVE_OPEN || UIPV4_UDP
  lastport = 1024;
#endif /* UIPV4_ACTIVE_OPEN || UIPV4_UDP */

#if UIPV4_UDP
  for(c = 0; c < UIPV4_UDP_CONNS; ++c) {
    uipv4_udp_conns[c].lport = 0;
  }
#endif /* UIPV4_UDP */ 
}
/*---------------------------------------------------------------------------*/
#if UIPV4_TCP && UIPV4_ACTIVE_OPEN
struct uipv4_conn *
uipv4_connect(uip_ip4addr_t *ripaddr, u16_t rport)
{
  register struct uipv4_conn *conn, *cconn;
  
  /* Find an unused local port. */
 again:
  ++lastport;

  if(lastport >= 32000) {
    lastport = 4096;
  }

  /* Check if this port is already in use, and if so try to find
     another one. */
  for(c = 0; c < UIPV4_CONNS; ++c) {
    conn = &uipv4_conns[c];
    if(conn->tcpstateflags != UIP_CLOSED &&
       conn->lport == uip_htons(lastport)) {
      goto again;
    }
  }

  conn = 0;
  for(c = 0; c < UIPV4_CONNS; ++c) {
    cconn = &uipv4_conns[c];
    if(cconn->tcpstateflags == UIP_CLOSED) {
      conn = cconn;
      break;
    }
    if(cconn->tcpstateflags == UIP_TIME_WAIT) {
      if(conn == 0 ||
	 cconn->timer > conn->timer) {
	conn = cconn;
      }
    }
  }

  if(conn == 0) {
    return 0;
  }
  
  conn->tcpstateflags = UIP_SYN_SENT;

  conn->snd_nxt[0] = iss[0];
  conn->snd_nxt[1] = iss[1];
  conn->snd_nxt[2] = iss[2];
  conn->snd_nxt[3] = iss[3];

  conn->initialmss = conn->mss = UIP_TCP_MSS;
  
  conn->len = 1;   /* TCP length of the SYN is one. */
  conn->nrtx = 0;
  conn->timer = 1; /* Send the SYN next time around. */
  conn->rto = UIP_RTO;
  conn->sa = 0;
  conn->sv = 16;   /* Initial value of the RTT variance. */
  conn->lport = uip_htons(lastport);
  conn->rport = rport;
  uipv4_ipaddr_copy(&conn->ripaddr, ripaddr);
  
  return conn;
}
#endif /* UIPV4_TCP && UIP_ACTIVE_OPEN */
/*---------------------------------------------------------------------------*/
#if UIPV4_UDP
struct uipv4_udp_conn *
uipv4_udp_new(const uip_ip4addr_t *ripaddr, u16_t rport)
{
  register struct uipv4_udp_conn *conn;
  
  /* Find an unused local port. */
 again:
  ++lastport;

  if(lastport >= 32000) {
    lastport = 4096;
  }
  
  for(c = 0; c < UIPV4_UDP_CONNS; ++c) {
    if(uipv4_udp_conns[c].lport == uip_htons(lastport)) {
      goto again;
    }
  }


  conn = 0;
  for(c = 0; c < UIPV4_UDP_CONNS; ++c) {
    if(uipv4_udp_conns[c].lport == 0) {
      conn = &uipv4_udp_conns[c];
      break;
    }
  }

  if(conn == 0) {
    return 0;
  }
  
  conn->lport = UIP_HTONS(lastport);
  conn->rport = rport;
  if(ripaddr == NULL) {
    memset(&conn->ripaddr, 0, sizeof(uip_ip4addr_t));
  } else {
    uipv4_ipaddr_copy(&conn->ripaddr, ripaddr);
  }
  conn->ttl = UIP_TTL;
  
  return conn;
}
#endif /* UIPV4_UDP */
/*---------------------------------------------------------------------------*/
#if UIPV4_TCP
void
uipv4_unlisten(u16_t port)
{
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(uipv4_listenports[c] == port) {
      uipv4_listenports[c] = 0;
      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
uipv4_listen(u16_t port)
{
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(uipv4_listenports[c] == 0) {
      uipv4_listenports[c] = port;
      return;
    }
  }
}

#endif /* UIPV4_TCP */
/*---------------------------------------------------------------------------*/
/* XXX: IP fragment reassembly: not well-tested. */

#if UIP_REASSEMBLY
#define UIP_REASS_BUFSIZE (UIP_BUFSIZE - UIP_LLH_LEN)
static u8_t uip_reassbuf[UIP_REASS_BUFSIZE];
static u8_t uip_reassbitmap[UIP_REASS_BUFSIZE / (8 * 8)];
static const u8_t bitmap_bits[8] = {0xff, 0x7f, 0x3f, 0x1f,
				    0x0f, 0x07, 0x03, 0x01};
static u16_t uip_reasslen;
static u8_t uip_reassflags;
#define UIP_REASS_FLAG_LASTFRAG 0x01
static u8_t uip_reasstmr;

#define IP_MF   0x20

static u8_t
uipv4_reass(void)
{
  u16_t offset, len;
  u16_t i;

  /* If ip_reasstmr is zero, no packet is present in the buffer, so we
     write the IP header of the fragment into the reassembly
     buffer. The timer is updated with the maximum age. */
  if(uip_reasstmr == 0) {
    memcpy(uip_reassbuf, &BUF->vhl, UIPV4_IPH_LEN);
    uip_reasstmr = UIP_REASS_MAXAGE;
    uip_reassflags = 0;
    /* Clear the bitmap. */
    memset(uip_reassbitmap, 0, sizeof(uip_reassbitmap));
  }

  /* Check if the incoming fragment matches the one currently present
     in the reasembly buffer. If so, we proceed with copying the
     fragment into the buffer. */
  if(BUF->srcipaddr[0] == FBUF->srcipaddr[0] &&
     BUF->srcipaddr[1] == FBUF->srcipaddr[1] &&
     BUF->destipaddr[0] == FBUF->destipaddr[0] &&
     BUF->destipaddr[1] == FBUF->destipaddr[1] &&
     BUF->ipid[0] == FBUF->ipid[0] &&
     BUF->ipid[1] == FBUF->ipid[1]) {

    len = (BUF->len[0] << 8) + BUF->len[1] - (BUF->vhl & 0x0f) * 4;
    offset = (((BUF->ipoffset[0] & 0x3f) << 8) + BUF->ipoffset[1]) * 8;

    /* If the offset or the offset + fragment length overflows the
       reassembly buffer, we discard the entire packet. */
    if(offset > UIP_REASS_BUFSIZE ||
       offset + len > UIP_REASS_BUFSIZE) {
      uip_reasstmr = 0;
      goto nullreturn;
    }

    /* Copy the fragment into the reassembly buffer, at the right
       offset. */
    memcpy(&uip_reassbuf[UIPV4_IPH_LEN + offset],
	   (char *)BUF + (int)((BUF->vhl & 0x0f) * 4),
	   len);
      
    /* Update the bitmap. */
    if(offset / (8 * 8) == (offset + len) / (8 * 8)) {
      /* If the two endpoints are in the same byte, we only update
	 that byte. */
	     
      uip_reassbitmap[offset / (8 * 8)] |=
	     bitmap_bits[(offset / 8 ) & 7] &
	     ~bitmap_bits[((offset + len) / 8 ) & 7];
    } else {
      /* If the two endpoints are in different bytes, we update the
	 bytes in the endpoints and fill the stuff inbetween with
	 0xff. */
      uip_reassbitmap[offset / (8 * 8)] |=
	bitmap_bits[(offset / 8 ) & 7];
      for(i = 1 + offset / (8 * 8); i < (offset + len) / (8 * 8); ++i) {
	uip_reassbitmap[i] = 0xff;
      }
      uip_reassbitmap[(offset + len) / (8 * 8)] |=
	~bitmap_bits[((offset + len) / 8 ) & 7];
    }
    
    /* If this fragment has the More Fragments flag set to zero, we
       know that this is the last fragment, so we can calculate the
       size of the entire packet. We also set the
       IP_REASS_FLAG_LASTFRAG flag to indicate that we have received
       the final fragment. */

    if((BUF->ipoffset[0] & IP_MF) == 0) {
      uip_reassflags |= UIP_REASS_FLAG_LASTFRAG;
      uip_reasslen = offset + len;
    }
    
    /* Finally, we check if we have a full packet in the buffer. We do
       this by checking if we have the last fragment and if all bits
       in the bitmap are set. */
    if(uip_reassflags & UIP_REASS_FLAG_LASTFRAG) {
      /* Check all bytes up to and including all but the last byte in
	 the bitmap. */
      for(i = 0; i < uip_reasslen / (8 * 8) - 1; ++i) {
	if(uip_reassbitmap[i] != 0xff) {
	  goto nullreturn;
	}
      }
      /* Check the last byte in the bitmap. It should contain just the
	 right amount of bits. */
      if(uip_reassbitmap[uip_reasslen / (8 * 8)] !=
	 (u8_t)~bitmap_bits[uip_reasslen / 8 & 7]) {
	goto nullreturn;
      }

      /* If we have come this far, we have a full packet in the
	 buffer, so we allocate a pbuf and copy the packet into it. We
	 also reset the timer. */
      uip_reasstmr = 0;
      memcpy(BUF, FBUF, uip_reasslen);

      /* Pretend to be a "normal" (i.e., not fragmented) IP packet
	 from now on. */
      BUF->ipoffset[0] = BUF->ipoffset[1] = 0;
      BUF->len[0] = uip_reasslen >> 8;
      BUF->len[1] = uip_reasslen & 0xff;
      BUF->ipchksum = 0;
      BUF->ipchksum = ~(uipv4_ipchksum());

      return uip_reasslen;
    }
  }

 nullreturn:
  return 0;
}
#endif /* UIP_REASSEMBLY */
/*---------------------------------------------------------------------------*/
#if UIPV4_TCP
static void
uip_add_rcv_nxt(u16_t n)
{
  uip_add32(uipv4_conn->rcv_nxt, n);
  uipv4_conn->rcv_nxt[0] = uip_acc32[0];
  uipv4_conn->rcv_nxt[1] = uip_acc32[1];
  uipv4_conn->rcv_nxt[2] = uip_acc32[2];
  uipv4_conn->rcv_nxt[3] = uip_acc32[3];
}
#endif /* UIPV4_TCP */
/*---------------------------------------------------------------------------*/
void
uipv4_process(u8_t flag)
{
#if UIPV4_TCP
  register struct uipv4_conn *uipv4_connr = uipv4_conn;
#endif /* UIPV4_TCP */

#if UIPV4_UDP
  if(flag == UIP_UDP_SEND_CONN) {
    goto udp_send;
  }
#endif /* UIPV4_UDP */
  
  uipv4_sappdata = uip_appdata = &uip_buf[UIPV4_IPTCPH_LEN + UIP_LLH_LEN];

  /* Check if we were invoked because of a poll request for a
     particular connection. */
  if(flag == UIP_POLL_REQUEST) {
#if UIPV4_TCP  	
    if((uipv4_connr->tcpstateflags & UIP_TS_MASK) == UIP_ESTABLISHED &&
    		!uipv4_outstanding(uipv4_connr)) {
			uipv4_flags = UIP_POLL;
			UIPV4_APPCALL();
			goto appsend;
#if UIPV4_ACTIVE_OPEN
    } else if((uipv4_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT) {
      /* In the SYN_SENT state, we retransmit out SYN. */
      BUF->flags = 0;
      goto tcp_send_syn;
#endif /* UIPV4_ACTIVE_OPEN */
    }
    goto drop;
#endif /* UIPV4_TCP */    
    /* Check if we were invoked because of the perodic timer fireing. */
  } else if(flag == UIP_TIMER) {
#if UIP_REASSEMBLY
    if(uip_reasstmr != 0) {
      --uip_reasstmr;
    }
#endif /* UIP_REASSEMBLY */

#if UIPV4_TCP
    /* Increase the initial sequence number. */
    if(++iss[3] == 0) {
      if(++iss[2] == 0) {
				if(++iss[1] == 0) {
				  ++iss[0];
				}
      }
    }

    /* Reset the length variables. */
    uip_len = 0;
    uipv4_slen = 0;

    /* Check if the connection is in a state in which we simply wait
       for the connection to time out. If so, we increase the
       connection's timer and remove the connection if it times
       out. */
    if(uipv4_connr->tcpstateflags == UIP_TIME_WAIT ||
    		uipv4_connr->tcpstateflags == UIP_FIN_WAIT_2) {

    	++(uipv4_connr->timer);
      if(uipv4_connr->timer == UIP_TIME_WAIT_TIMEOUT) {
				uipv4_connr->tcpstateflags = UIP_CLOSED;
      }
    } else if(uipv4_connr->tcpstateflags != UIP_CLOSED) {
      /* If the connection has outstanding data, we increase the
	 			 connection's timer and see if it has reached the RTO value
	 			 in which case we retransmit. */

      if(uipv4_outstanding(uipv4_connr)) {
				if(uipv4_connr->timer-- == 0) {
	  			if(uipv4_connr->nrtx == UIP_MAXRTX ||
	     				((uipv4_connr->tcpstateflags == UIP_SYN_SENT ||
	       			uipv4_connr->tcpstateflags == UIP_SYN_RCVD) &&
	      			uipv4_connr->nrtx == UIP_MAXSYNRTX)) {
	      			
		    		uipv4_connr->tcpstateflags = UIP_CLOSED;
	
		    		/* We call UIPV4_APPCALL() with uipv4_flags set to
		       		 UIP_TIMEDOUT to inform the application that the
		       		 connection has timed out. */
		    		uipv4_flags = UIP_TIMEDOUT;
		    		UIPV4_APPCALL();
	
		    		/* We also send a reset packet to the remote host. */
		    		BUF->flags = TCP_RST | TCP_ACK;
		    		goto tcp_send_nodata;
		  		}
	
		  		/* Exponential backoff. */
		  		uipv4_connr->timer = UIP_RTO << (uipv4_connr->nrtx > 4 ?
						 4 :
						 uipv4_connr->nrtx);
		  		++(uipv4_connr->nrtx);
		  
		  		/* Ok, so we need to retransmit. We do this differently
		     		 depending on which state we are in. In ESTABLISHED, we
		     		 call upon the application so that it may prepare the
		     		 data for the retransmit. In SYN_RCVD, we resend the
		     		 SYNACK that we sent earlier and in LAST_ACK we have to
		     		 retransmit our FINACK. */
		  		UIP_STAT(++uip_stat.tcp.rexmit);
		  		switch(uipv4_connr->tcpstateflags & UIP_TS_MASK) {
		  		case UIP_SYN_RCVD:
		    		/* In the SYN_RCVD state, we should retransmit our SYNACK. */
		    		goto tcp_send_synack;
		    
	#if UIPV4_ACTIVE_OPEN
		  		case UIP_SYN_SENT:
		    		/* In the SYN_SENT state, we retransmit out SYN. */
		    		BUF->flags = 0;
		    		goto tcp_send_syn;
	#endif /* UIPV4_ACTIVE_OPEN */
		    
		  		case UIP_ESTABLISHED:
		    		/* In the ESTABLISHED state, we call upon the application
	             to do the actual retransmit after which we jump into
	             the code for sending out the packet (the apprexmit
	             label). */
		    		uipv4_flags = UIP_REXMIT;
		    		UIPV4_APPCALL();
		    		goto apprexmit;
		    
				  case UIP_FIN_WAIT_1:
				  case UIP_CLOSING:
				  case UIP_LAST_ACK:
				    /* In all these states we should retransmit a FINACK. */
				    goto tcp_send_finack;
		    
		  		}
				}
	    } else if((uipv4_connr->tcpstateflags & UIP_TS_MASK) == UIP_ESTABLISHED) {
				/* If there was no need for a retransmission, we poll the
	         application for new data. */
				uipv4_flags = UIP_POLL;
				UIPV4_APPCALL();
				goto appsend;
	    }
	  }
	  goto drop;
#endif /* UIPV4_TCP */	  
  }
#if UIPV4_UDP
  if(flag == UIP_UDP_TIMER) {
    if(uipv4_udp_conn->lport != 0) {
      uipv4_conn = NULL;
      uipv4_sappdata = uip_appdata = &uip_buf[UIP_LLH_LEN + UIPV4_IPUDPH_LEN];
      uip_len = uipv4_slen = 0;
      uipv4_flags = UIP_POLL;
      UIPV4_UDP_APPCALL();
      goto udp_send;
    } else {
      goto drop;
    }
  }
#endif

  /* This is where the input processing starts. */
  UIP_STAT(++uip_stat.ip.recv);

  /* Start of IP input header processing code. */
  
  /* Check validity of the IP header. */
  if(BUF->vhl != 0x45)  { /* IP version and header length. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.vhlerr);
    UIP_LOG("ip: invalid version or header length.");
    goto drop;
  }
  
  /* Check the size of the packet. If the size reported to us in
     uip_len is smaller the size reported in the IP header, we assume
     that the packet has been corrupted in transit. If the size of
     uip_len is larger than the size reported in the IP packet header,
     the packet has been padded and we set uip_len to the correct
     value.. */

  if((BUF->len[0] << 8) + BUF->len[1] <= uip_len) {
    uip_len = (BUF->len[0] << 8) + BUF->len[1];
  } else {
    UIP_LOG("ip: packet shorter than reported in IP header.");
    goto drop;
  }

  /* Check the fragment flag. */
  if((BUF->ipoffset[0] & 0x3f) != 0 ||
     BUF->ipoffset[1] != 0) {
#if UIP_REASSEMBLY
    uip_len = uipv4_reass();
    if(uip_len == 0) {
      goto drop;
    }
#else /* UIP_REASSEMBLY */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.fragerr);
    UIP_LOG("ip: fragment dropped.");
    goto drop;
#endif /* UIP_REASSEMBLY */
  }

  /* If IP broadcast support is configured, we check for a broadcast
     UDP packet, which may be destined to us. */
#if UIPV4_BROADCAST
  DEBUG_PRINTF("UDP IP checksum 0x%04x\n", uipv4_ipchksum());
  if(BUF->proto == UIP_PROTO_UDP &&
  		(uipv4_ipaddr_cmp(&BUF->destipaddr, &uipv4_broadcast_addr) ||
			(BUF->destipaddr.u8[0] & 224) == 224)) {  /* XXX this is a
						     hack to be able
						     to receive UDP
						     multicast
						     packets. We check
						     for the bit
						     pattern of the
						     multicast
						     prefix. */
    goto udp_input;
  }
#endif /* UIPV4_BROADCAST */
    
  /* Check if the packet is destined for our IP address. */
  if(!uipv4_ipaddr_cmp(&BUF->destipaddr, &uipv4_hostaddr)) {
    UIP_STAT(++uip_stat.ip.drop);
    goto drop;
  }

  if(uipv4_ipchksum() != 0xffff) { /* Compute and check the IP header
				    checksum. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.chkerr);
    UIP_LOG("ip: bad checksum.");
    goto drop;
  }

#if UIPV4_TCP
  if(BUF->proto == UIP_PROTO_TCP) { /* Check for TCP packet. If so,
				       proceed with TCP input
				       processing. */
    goto tcp_input;
  }
#endif /* UIPV4_TCP */

#if UIPV4_UDP
  if(BUF->proto == UIP_PROTO_UDP) {
    goto udp_input;
  }
#endif /* UIPV4_UDP */

  /* ICMPv4 processing code follows. */
  if(BUF->proto != UIP_PROTO_ICMP) { /* We only allow ICMP packets from here. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.protoerr);
    UIP_LOG("ip: neither tcp nor icmp.");
    goto drop;
  }

  UIP_STAT(++uip_stat.icmp.recv);

  /* ICMP echo (i.e., ping) processing. This is simple, we only change
     the ICMP type from ECHO to ECHO_REPLY and adjust the ICMP
     checksum before we return the packet. */
  if(ICMPBUF->type != ICMP_ECHO) {
    UIP_STAT(++uip_stat.icmp.drop);
    UIP_STAT(++uip_stat.icmp.typeerr);
    UIP_LOG("icmp: not icmp echo.");
    goto drop;
  }

  ICMPBUF->type = ICMP_ECHO_REPLY;

  if(ICMPBUF->icmpchksum >= UIP_HTONS(0xffff - (ICMP_ECHO << 8))) {
    ICMPBUF->icmpchksum += UIP_HTONS(ICMP_ECHO << 8) + 1;
  } else {
    ICMPBUF->icmpchksum += UIP_HTONS(ICMP_ECHO << 8);
  }

  /* Swap IP addresses. */
  uipv4_ipaddr_copy(&BUF->destipaddr, &BUF->srcipaddr);
  uipv4_ipaddr_copy(&BUF->srcipaddr, &uipv4_hostaddr);

  UIP_STAT(++uip_stat.icmp.sent);
  BUF->ttl = UIP_TTL;
  goto ip_send_nolen;

  /* End of IPv4 input header processing code. */

#if UIPV4_UDP
  /* UDP input processing. */
 udp_input:
  /* UDP processing is really just a hack. We don't do anything to the
     UDP/IP headers, but let the UDP application do all the hard
     work. If the application sets uipv4_slen, it has a packet to
     send. */
  uip_len = uip_len - UIPV4_IPUDPH_LEN;
#if UIPV4_UDP_CHECKSUMS
  uip_appdata = &uip_buf[UIP_LLH_LEN + UIPV4_IPUDPH_LEN];
  if(UDPBUF->udpchksum != 0 && uipv4_udpchksum() != 0xffff) {
    UIP_STAT(++uip_stat.udp.drop);
    UIP_STAT(++uip_stat.udp.chkerr);
    UIP_LOG("udp: bad checksum.");
    goto drop;
  }
#endif /* UIPV4_UDP_CHECKSUMS */

  /* Make sure that the UDP destination port number is not zero. */
  if(UDPBUF->destport == 0) {
    UIP_LOG("udp: zero port.");
    goto drop;
  }

  /* Demultiplex this UDP packet between the UDP "connections". */
  for(uipv4_udp_conn = &uipv4_udp_conns[0];
      uipv4_udp_conn < &uipv4_udp_conns[UIPV4_UDP_CONNS];
      ++uipv4_udp_conn) {
    /* If the local UDP port is non-zero, the connection is considered
       to be used. If so, the local port number is checked against the
       destination port number in the received packet. If the two port
       numbers match, the remote port number is checked if the
       connection is bound to a remote port. Finally, if the
       connection is bound to a remote IP address, the source IP
       address of the packet is checked. */
    if(uipv4_udp_conn->lport != 0 && UDPBUF->destport == uipv4_udp_conn->lport &&
       	(uipv4_udp_conn->rport == 0 || UDPBUF->srcport == uipv4_udp_conn->rport) &&
       	(uipv4_ipaddr_cmp(&uipv4_udp_conn->ripaddr, &uipv4_all_zeroes_addr) ||
				uipv4_ipaddr_cmp(&uipv4_udp_conn->ripaddr, &uipv4_broadcast_addr) ||
				uipv4_ipaddr_cmp(&BUF->srcipaddr, &uipv4_udp_conn->ripaddr))) {
      goto udp_found;
    }
  }
  UIP_LOG("udp: no matching connection found");
#if UIP_CONF_ICMP_DEST_UNREACH
  /* Copy fields from packet header into payload of this ICMP packet. */
  memcpy(&(ICMPBUF->payload[0]), ICMPBUF, UIPV4_IPH_LEN + 8);

  /* Set the ICMP type and code. */
  ICMPBUF->type = ICMP_DEST_UNREACHABLE;
  ICMPBUF->icode = ICMP_PORT_UNREACHABLE;

  /* Calculate the ICMP checksum. */
  ICMPBUF->icmpchksum = 0;
  ICMPBUF->icmpchksum = ~uip_chksum((u16_t *)&(ICMPBUF->type), 36);

  /* Set the IP destination address to be the source address of the
     original packet. */
  uipv4_ipaddr_copy(&BUF->destipaddr, &BUF->srcipaddr);

  /* Set our IP address as the source address. */
  uipv4_ipaddr_copy(&BUF->srcipaddr, &uipv4_hostaddr);

  /* The size of the ICMP destination unreachable packet is 36 + the
     size of the IP header (20) = 56. */
  uip_len = 36 + UIPV4_IPH_LEN;
  ICMPBUF->len[0] = 0;
  ICMPBUF->len[1] = (u8_t)uip_len;
  ICMPBUF->ttl = UIP_TTL;
  ICMPBUF->proto = UIP_PROTO_ICMP;

  goto ip_send_nolen;
#else /* UIP_CONF_ICMP_DEST_UNREACH */
  goto drop;
#endif /* UIP_CONF_ICMP_DEST_UNREACH */
  
 udp_found:
  uipv4_conn = NULL;
  uipv4_flags = UIP_NEWDATA;
  uipv4_sappdata = uip_appdata = &uip_buf[UIP_LLH_LEN + UIPV4_IPUDPH_LEN];
  uipv4_slen = 0;
  UIPV4_UDP_APPCALL();

 udp_send:
  if(uipv4_slen == 0) {
    goto drop;
  }
  uip_len = uipv4_slen + UIPV4_IPUDPH_LEN;

  BUF->len[0] = (uip_len >> 8);
  BUF->len[1] = (uip_len & 0xff);

  BUF->ttl = uipv4_udp_conn->ttl;
  BUF->proto = UIP_PROTO_UDP;

  UDPBUF->udplen = UIP_HTONS(uipv4_slen + UIP_UDPH_LEN);
  UDPBUF->udpchksum = 0;

  BUF->srcport  = uipv4_udp_conn->lport;
  BUF->destport = uipv4_udp_conn->rport;

  uipv4_ipaddr_copy(&BUF->srcipaddr, &uipv4_hostaddr);
  uipv4_ipaddr_copy(&BUF->destipaddr, &uipv4_udp_conn->ripaddr);
   
  uip_appdata = &uip_buf[UIP_LLH_LEN + UIPV4_IPTCPH_LEN];

#if UIPV4_UDP_CHECKSUMS
  /* Calculate UDP checksum. */
  UDPBUF->udpchksum = ~(uipv4_udpchksum());
  if(UDPBUF->udpchksum == 0) {
    UDPBUF->udpchksum = 0xffff;
  }
#endif /* UIPV4_UDP_CHECKSUMS */
  
  goto ip_send_nolen;
#endif /* UIPV4_UDP */

#if UIPV4_TCP  
  /* TCP input processing. */
 tcp_input:
  UIP_STAT(++uip_stat.tcp.recv);

  /* Start of TCP input header processing code. */
  
  if(uipv4_tcpchksum() != 0xffff) {   /* Compute and check the TCP
				       checksum. */
    UIP_STAT(++uip_stat.tcp.drop);
    UIP_STAT(++uip_stat.tcp.chkerr);
    UIP_LOG("tcp: bad checksum.");
    goto drop;
  }

  /* Make sure that the TCP port number is not zero. */
  if(BUF->destport == 0 || BUF->srcport == 0) {
    UIP_LOG("tcp: zero port.");
    goto drop;
  }
  
  /* Demultiplex this segment. */
  /* First check any active connections. */
  for(uipv4_connr = &uipv4_conns[0]; uipv4_connr <= &uipv4_conns[UIPV4_CONNS - 1];
      ++uipv4_connr) {
    if(uipv4_connr->tcpstateflags != UIP_CLOSED &&
       BUF->destport == uipv4_connr->lport &&
       BUF->srcport == uipv4_connr->rport &&
       uipv4_ipaddr_cmp(&BUF->srcipaddr, &uipv4_connr->ripaddr)) {
      goto found;
    }
  }

  /* If we didn't find and active connection that expected the packet,
     either this packet is an old duplicate, or this is a SYN packet
     destined for a connection in LISTEN. If the SYN flag isn't set,
     it is an old packet and we send a RST. */
  if((BUF->flags & TCP_CTL) != TCP_SYN) {
    goto reset;
  }
  
  tmp16 = BUF->destport;
  /* Next, check listening connections. */
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(tmp16 == uipv4_listenports[c]) {
      goto found_listen;
    }
  }
  
  /* No matching connection found, so we send a RST packet. */
  UIP_STAT(++uip_stat.tcp.synrst);

 reset:
  /* We do not send resets in response to resets. */
  if(BUF->flags & TCP_RST) {
    goto drop;
  }

  UIP_STAT(++uip_stat.tcp.rst);
  
  BUF->flags = TCP_RST | TCP_ACK;
  uip_len = UIPV4_IPTCPH_LEN;
  BUF->tcpoffset = 5 << 4;

  /* Flip the seqno and ackno fields in the TCP header. */
  c = BUF->seqno[3];
  BUF->seqno[3] = BUF->ackno[3];
  BUF->ackno[3] = c;
  
  c = BUF->seqno[2];
  BUF->seqno[2] = BUF->ackno[2];
  BUF->ackno[2] = c;
  
  c = BUF->seqno[1];
  BUF->seqno[1] = BUF->ackno[1];
  BUF->ackno[1] = c;
  
  c = BUF->seqno[0];
  BUF->seqno[0] = BUF->ackno[0];
  BUF->ackno[0] = c;

  /* We also have to increase the sequence number we are
     acknowledging. If the least significant byte overflowed, we need
     to propagate the carry to the other bytes as well. */
  if(++BUF->ackno[3] == 0) {
    if(++BUF->ackno[2] == 0) {
      if(++BUF->ackno[1] == 0) {
	++BUF->ackno[0];
      }
    }
  }
 
  /* Swap port numbers. */
  tmp16 = BUF->srcport;
  BUF->srcport = BUF->destport;
  BUF->destport = tmp16;
  
  /* Swap IP addresses. */
  uipv4_ipaddr_copy(&BUF->destipaddr, &BUF->srcipaddr);
  uipv4_ipaddr_copy(&BUF->srcipaddr, &uipv4_hostaddr);
  
  /* And send out the RST packet! */
  goto tcp_send_noconn;

  /* This label will be jumped to if we matched the incoming packet
     with a connection in LISTEN. In that case, we should create a new
     connection and send a SYNACK in return. */
 found_listen:
  /* First we check if there are any connections avaliable. Unused
     connections are kept in the same table as used connections, but
     unused ones have the tcpstate set to CLOSED. Also, connections in
     TIME_WAIT are kept track of and we'll use the oldest one if no
     CLOSED connections are found. Thanks to Eddie C. Dost for a very
     nice algorithm for the TIME_WAIT search. */
  uipv4_connr = 0;
  for(c = 0; c < UIPV4_CONNS; ++c) {
    if(uipv4_conns[c].tcpstateflags == UIP_CLOSED) {
      uipv4_connr = &uipv4_conns[c];
      break;
    }
    if(uipv4_conns[c].tcpstateflags == UIP_TIME_WAIT) {
      if(uipv4_connr == 0 ||
	 				uipv4_conns[c].timer > uipv4_connr->timer) {
				uipv4_connr = &uipv4_conns[c];
      }
    }
  }

  if(uipv4_connr == 0) {
    /* All connections are used already, we drop packet and hope that
       the remote end will retransmit the packet at a time when we
       have more spare connections. */
    UIP_STAT(++uip_stat.tcp.syndrop);
    UIP_LOG("tcp: found no unused connections.");
    goto drop;
  }
  uipv4_conn = uipv4_connr;
  
  /* Fill in the necessary fields for the new connection. */
  uipv4_connr->rto = uipv4_connr->timer = UIP_RTO;
  uipv4_connr->sa = 0;
  uipv4_connr->sv = 4;
  uipv4_connr->nrtx = 0;
  uipv4_connr->lport = BUF->destport;
  uipv4_connr->rport = BUF->srcport;
  uipv4_ipaddr_copy(&uipv4_connr->ripaddr, &BUF->srcipaddr);
  uipv4_connr->tcpstateflags = UIP_SYN_RCVD;

  uipv4_connr->snd_nxt[0] = iss[0];
  uipv4_connr->snd_nxt[1] = iss[1];
  uipv4_connr->snd_nxt[2] = iss[2];
  uipv4_connr->snd_nxt[3] = iss[3];
  uipv4_connr->len = 1;

  /* rcv_nxt should be the seqno from the incoming packet + 1. */
  uipv4_connr->rcv_nxt[3] = BUF->seqno[3];
  uipv4_connr->rcv_nxt[2] = BUF->seqno[2];
  uipv4_connr->rcv_nxt[1] = BUF->seqno[1];
  uipv4_connr->rcv_nxt[0] = BUF->seqno[0];
  uip_add_rcv_nxt(1);

  /* Parse the TCP MSS option, if present. */
  if((BUF->tcpoffset & 0xf0) > 0x50) {
    for(c = 0; c < ((BUF->tcpoffset >> 4) - 5) << 2 ;) {
      opt = uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + c];
      if(opt == TCP_OPT_END) {
				/* End of options. */
				break;
      } else if(opt == TCP_OPT_NOOP) {
				/* NOP option. */
				++c;
      } else if(opt == TCP_OPT_MSS &&
					uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == TCP_OPT_MSS_LEN) {
				/* An MSS option with the right option length. */
				tmp16 = ((u16_t)uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 2 + c] << 8) |
	  						(u16_t)uip_buf[UIPV4_IPTCPH_LEN + UIP_LLH_LEN + 3 + c];
				uipv4_connr->initialmss = uipv4_connr->mss =
	  		tmp16 > UIP_TCP_MSS ? UIP_TCP_MSS : tmp16;
	
				/* And we are done processing options. */
				break;
      } else {
				/* All other options have a length field, so that we easily
				   can skip past them. */
				if(uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0) {
	  			/* If the length field is zero, the options are malformed
	     			 and we don't process them further. */
	  			break;
				}
				c += uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 1 + c];
      }
    }
  }
  
  /* Our response will be a SYNACK. */
#if UIPV4_ACTIVE_OPEN
 tcp_send_synack:
  BUF->flags = TCP_ACK;
  
 tcp_send_syn:
  BUF->flags |= TCP_SYN;
#else /* UIPV4_ACTIVE_OPEN */
 tcp_send_synack:
  BUF->flags = TCP_SYN | TCP_ACK;
#endif /* UIPV4_ACTIVE_OPEN */
  
  /* We send out the TCP Maximum Segment Size option with our
     SYNACK. */
  BUF->optdata[0] = TCP_OPT_MSS;
  BUF->optdata[1] = TCP_OPT_MSS_LEN;
  BUF->optdata[2] = (UIP_TCP_MSS) / 256;
  BUF->optdata[3] = (UIP_TCP_MSS) & 255;
  uip_len = UIPV4_IPTCPH_LEN + TCP_OPT_MSS_LEN;
  BUF->tcpoffset = ((UIP_TCPH_LEN + TCP_OPT_MSS_LEN) / 4) << 4;
  goto tcp_send;

  /* This label will be jumped to if we found an active connection. */
 found:
  uipv4_conn = uipv4_connr;
  uipv4_flags = 0;
  /* We do a very naive form of TCP reset processing; we just accept
     any RST and kill our connection. We should in fact check if the
     sequence number of this reset is wihtin our advertised window
     before we accept the reset. */
  if(BUF->flags & TCP_RST) {
    uipv4_connr->tcpstateflags = UIP_CLOSED;
    UIP_LOG("tcp: got reset, aborting connection.");
    uipv4_flags = UIP_ABORT;
    UIPV4_APPCALL();
    goto drop;
  }
  /* Calculate the length of the data, if the application has sent
     any data to us. */
  c = (BUF->tcpoffset >> 4) << 2;
  /* uip_len will contain the length of the actual TCP data. This is
     calculated by subtracing the length of the TCP header (in
     c) and the length of the IP header (20 bytes). */
  uip_len = uip_len - c - UIPV4_IPH_LEN;

  /* First, check if the sequence number of the incoming packet is
     what we're expecting next. If not, we send out an ACK with the
     correct numbers in, unless we are in the SYN_RCVD state and
     receive a SYN, in which case we should retransmit our SYNACK
     (which is done futher down). */
  if(!((((uipv4_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT) &&
	((BUF->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))) ||
       (((uipv4_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_RCVD) &&
	((BUF->flags & TCP_CTL) == TCP_SYN)))) {
    if((uip_len > 0 || ((BUF->flags & (TCP_SYN | TCP_FIN)) != 0)) &&
       (BUF->seqno[0] != uipv4_connr->rcv_nxt[0] ||
	BUF->seqno[1] != uipv4_connr->rcv_nxt[1] ||
	BUF->seqno[2] != uipv4_connr->rcv_nxt[2] ||
	BUF->seqno[3] != uipv4_connr->rcv_nxt[3])) {
      goto tcp_send_ack;
    }
  }

  /* Next, check if the incoming segment acknowledges any outstanding
     data. If so, we update the sequence number, reset the length of
     the outstanding data, calculate RTT estimations, and reset the
     retransmission timer. */
  if((BUF->flags & TCP_ACK) && uipv4_outstanding(uipv4_connr)) {
    uip_add32(uipv4_connr->snd_nxt, uipv4_connr->len);

    if(BUF->ackno[0] == uip_acc32[0] &&
       BUF->ackno[1] == uip_acc32[1] &&
       BUF->ackno[2] == uip_acc32[2] &&
       BUF->ackno[3] == uip_acc32[3]) {
      /* Update sequence number. */
      uipv4_connr->snd_nxt[0] = uip_acc32[0];
      uipv4_connr->snd_nxt[1] = uip_acc32[1];
      uipv4_connr->snd_nxt[2] = uip_acc32[2];
      uipv4_connr->snd_nxt[3] = uip_acc32[3];
	
      /* Do RTT estimation, unless we have done retransmissions. */
      if(uipv4_connr->nrtx == 0) {
				signed char m;
				m = uipv4_connr->rto - uipv4_connr->timer;
				/* This is taken directly from VJs original code in his paper */
				m = m - (uipv4_connr->sa >> 3);
				uipv4_connr->sa += m;
				if(m < 0) {
				  m = -m;
				}
				m = m - (uipv4_connr->sv >> 2);
				uipv4_connr->sv += m;
				uipv4_connr->rto = (uipv4_connr->sa >> 3) + uipv4_connr->sv;
      }
      /* Set the acknowledged flag. */
      uipv4_flags = UIP_ACKDATA;
      /* Reset the retransmission timer. */
      uipv4_connr->timer = uipv4_connr->rto;

      /* Reset length of outstanding data. */
      uipv4_connr->len = 0;
    }
    
  }

  /* Do different things depending on in what state the connection is. */
  switch(uipv4_connr->tcpstateflags & UIP_TS_MASK) {
    /* CLOSED and LISTEN are not handled here. CLOSE_WAIT is not
	implemented, since we force the application to close when the
	peer sends a FIN (hence the application goes directly from
	ESTABLISHED to LAST_ACK). */
  case UIP_SYN_RCVD:
    /* In SYN_RCVD we have sent out a SYNACK in response to a SYN, and
       we are waiting for an ACK that acknowledges the data we sent
       out the last time. Therefore, we want to have the UIP_ACKDATA
       flag set. If so, we enter the ESTABLISHED state. */
    if(uipv4_flags & UIP_ACKDATA) {
      uipv4_connr->tcpstateflags = UIP_ESTABLISHED;
      uipv4_flags = UIP_CONNECTED;
      uipv4_connr->len = 0;
      if(uip_len > 0) {
        uipv4_flags |= UIP_NEWDATA;
        uip_add_rcv_nxt(uip_len);
      }
      uipv4_slen = 0;
      UIPV4_APPCALL();
      goto appsend;
    }
    /* We need to retransmit the SYNACK */
    if((BUF->flags & TCP_CTL) == TCP_SYN) {
      goto tcp_send_synack;
    }
    goto drop;
#if UIPV4_ACTIVE_OPEN
  case UIP_SYN_SENT:
    /* In SYN_SENT, we wait for a SYNACK that is sent in response to
       our SYN. The rcv_nxt is set to sequence number in the SYNACK
       plus one, and we send an ACK. We move into the ESTABLISHED
       state. */
    if((uipv4_flags & UIP_ACKDATA) &&
       (BUF->flags & TCP_CTL) == (TCP_SYN | TCP_ACK)) {

      /* Parse the TCP MSS option, if present. */
      if((BUF->tcpoffset & 0xf0) > 0x50) {
				for(c = 0; c < ((BUF->tcpoffset >> 4) - 5) << 2 ;) {
				  opt = uip_buf[UIPV4_IPTCPH_LEN + UIP_LLH_LEN + c];
				  if(opt == TCP_OPT_END) {
				    /* End of options. */
				    break;
				  } else if(opt == TCP_OPT_NOOP) {
				    ++c;
				    /* NOP option. */
				  } else if(opt == TCP_OPT_MSS &&
					    uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == TCP_OPT_MSS_LEN) {
				    /* An MSS option with the right option length. */
				    tmp16 = (uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 2 + c] << 8) |
				      uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 3 + c];
				    uipv4_connr->initialmss =
				      uipv4_connr->mss = tmp16 > UIP_TCP_MSS? UIP_TCP_MSS: tmp16;
			
				    /* And we are done processing options. */
				    break;
				  } else {
				    /* All other options have a length field, so that we easily
				       can skip past them. */
				    if(uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0) {
				      /* If the length field is zero, the options are malformed
					 and we don't process them further. */
				      break;
				    }
				    c += uip_buf[UIPV4_TCPIP_HLEN + UIP_LLH_LEN + 1 + c];
				  }
				}
      }
      uipv4_connr->tcpstateflags = UIP_ESTABLISHED;
      uipv4_connr->rcv_nxt[0] = BUF->seqno[0];
      uipv4_connr->rcv_nxt[1] = BUF->seqno[1];
      uipv4_connr->rcv_nxt[2] = BUF->seqno[2];
      uipv4_connr->rcv_nxt[3] = BUF->seqno[3];
      uip_add_rcv_nxt(1);
      uipv4_flags = UIP_CONNECTED | UIP_NEWDATA;
      uipv4_connr->len = 0;
      uip_len = 0;
      uipv4_slen = 0;
      UIPV4_APPCALL();
      goto appsend;
    }
    /* Inform the application that the connection failed */
    uipv4_flags = UIP_ABORT;
    UIPV4_APPCALL();
    /* The connection is closed after we send the RST */
    uipv4_conn->tcpstateflags = UIP_CLOSED;
    goto reset;
#endif /* UIPV4_ACTIVE_OPEN */
    
  case UIP_ESTABLISHED:
    /* In the ESTABLISHED state, we call upon the application to feed
    data into the uip_buf. If the UIP_ACKDATA flag is set, the
    application should put new data into the buffer, otherwise we are
    retransmitting an old segment, and the application should put that
    data into the buffer.

    If the incoming packet is a FIN, we should close the connection on
    this side as well, and we send out a FIN and enter the LAST_ACK
    state. We require that there is no outstanding data; otherwise the
    sequence numbers will be screwed up. */

    if(BUF->flags & TCP_FIN && !(uipv4_connr->tcpstateflags & UIP_STOPPED)) {
      if(uipv4_outstanding(uipv4_connr)) {
				goto drop;
      }
      uip_add_rcv_nxt(1 + uip_len);
      uipv4_flags |= UIP_CLOSE;
      if(uip_len > 0) {
				uipv4_flags |= UIP_NEWDATA;
      }
      UIPV4_APPCALL();
      uipv4_connr->len = 1;
      uipv4_connr->tcpstateflags = UIP_LAST_ACK;
      uipv4_connr->nrtx = 0;
 tcp_send_finack:
      BUF->flags = TCP_FIN | TCP_ACK;
      goto tcp_send_nodata;
    }

    /* Check the URG flag. If this is set, the segment carries urgent
       data that we must pass to the application. */
    if((BUF->flags & TCP_URG) != 0) {
#if UIP_URGDATA > 0
      uip_urglen = (BUF->urgp[0] << 8) | BUF->urgp[1];
      if(uip_urglen > uip_len) {
				/* There is more urgent data in the next segment to come. */
				uip_urglen = uip_len;
      }
      uip_add_rcv_nxt(uip_urglen);
      uip_len -= uip_urglen;
      uip_urgdata = uip_appdata;
      uip_appdata += uip_urglen;
    } else {
      uip_urglen = 0;
#else /* UIP_URGDATA > 0 */
      uip_appdata = ((char *)uip_appdata) + ((BUF->urgp[0] << 8) | BUF->urgp[1]);
      uip_len -= (BUF->urgp[0] << 8) | BUF->urgp[1];
#endif /* UIP_URGDATA > 0 */
    }

    /* If uip_len > 0 we have TCP data in the packet, and we flag this
       by setting the UIP_NEWDATA flag and update the sequence number
       we acknowledge. If the application has stopped the dataflow
       using uip_stop(), we must not accept any data packets from the
       remote host. */
    if(uip_len > 0 && !(uipv4_connr->tcpstateflags & UIP_STOPPED)) {
      uipv4_flags |= UIP_NEWDATA;
      uip_add_rcv_nxt(uip_len);
    }

    /* Check if the available buffer space advertised by the other end
       is smaller than the initial MSS for this connection. If so, we
       set the current MSS to the window size to ensure that the
       application does not send more data than the other end can
       handle.

       If the remote host advertises a zero window, we set the MSS to
       the initial MSS so that the application will send an entire MSS
       of data. This data will not be acknowledged by the receiver,
       and the application will retransmit it. This is called the
       "persistent timer" and uses the retransmission mechanim.
    */
    tmp16 = ((u16_t)BUF->wnd[0] << 8) + (u16_t)BUF->wnd[1];
    if(tmp16 > uipv4_connr->initialmss ||
       tmp16 == 0) {
      tmp16 = uipv4_connr->initialmss;
    }
    uipv4_connr->mss = tmp16;

    /* If this packet constitutes an ACK for outstanding data (flagged
       by the UIP_ACKDATA flag, we should call the application since it
       might want to send more data. If the incoming packet had data
       from the peer (as flagged by the UIP_NEWDATA flag), the
       application must also be notified.

       When the application is called, the global variable uip_len
       contains the length of the incoming data. The application can
       access the incoming data through the global pointer
       uip_appdata, which usually points UIPV4_IPTCPH_LEN + UIP_LLH_LEN
       bytes into the uip_buf array.

       If the application wishes to send any data, this data should be
       put into the uip_appdata and the length of the data should be
       put into uip_len. If the application don't have any data to
       send, uip_len must be set to 0. */
    if(uipv4_flags & (UIP_NEWDATA | UIP_ACKDATA)) {
      uipv4_slen = 0;
      UIPV4_APPCALL();

 appsend:
      
      if(uipv4_flags & UIP_ABORT) {
				uipv4_slen = 0;
				uipv4_connr->tcpstateflags = UIP_CLOSED;
				BUF->flags = TCP_RST | TCP_ACK;
				goto tcp_send_nodata;
      }

      if(uipv4_flags & UIP_CLOSE) {
				uipv4_slen = 0;
				uipv4_connr->len = 1;
				uipv4_connr->tcpstateflags = UIP_FIN_WAIT_1;
				uipv4_connr->nrtx = 0;
				BUF->flags = TCP_FIN | TCP_ACK;
				goto tcp_send_nodata;
      }

      /* If uipv4_slen > 0, the application has data to be sent. */
      if(uipv4_slen > 0) {

				/* If the connection has acknowledged data, the contents of
				   the ->len variable should be discarded. */
				if((uipv4_flags & UIP_ACKDATA) != 0) {
				  uipv4_connr->len = 0;
				}
			
				/* If the ->len variable is non-zero the connection has
				   already data in transit and cannot send anymore right
				   now. */
				if(uipv4_connr->len == 0) {
			
				  /* The application cannot send more than what is allowed by
				     the mss (the minumum of the MSS and the available
				     window). */
				  if(uipv4_slen > uipv4_connr->mss) {
				    uipv4_slen = uipv4_connr->mss;
				  }
			
				  /* Remember how much data we send out now so that we know
				     when everything has been acknowledged. */
				  uipv4_connr->len = uipv4_slen;
				} else {
			
				  /* If the application already had unacknowledged data, we
				     make sure that the application does not send (i.e.,
				     retransmit) out more than it previously sent out. */
				  uipv4_slen = uipv4_connr->len;
				}
      }
      uipv4_connr->nrtx = 0;
 apprexmit:
      uip_appdata = uipv4_sappdata;
      
      /* If the application has data to be sent, or if the incoming
         packet had new data in it, we must send out a packet. */
      if(uipv4_slen > 0 && uipv4_connr->len > 0) {
				/* Add the length of the IP and TCP headers. */
				uip_len = uipv4_connr->len + UIPV4_TCPIP_HLEN;
				/* We always set the ACK flag in response packets. */
				BUF->flags = TCP_ACK | TCP_PSH;
				/* Send the packet. */
				goto tcp_send_noopts;
      }
      /* If there is no data to send, just send out a pure ACK if
	 			 there is newdata. */
      if(uipv4_flags & UIP_NEWDATA) {
				uip_len = UIPV4_TCPIP_HLEN;
				BUF->flags = TCP_ACK;
				goto tcp_send_noopts;
      }
    }
    goto drop;
  case UIP_LAST_ACK:
    /* We can close this connection if the peer has acknowledged our
       FIN. This is indicated by the UIP_ACKDATA flag. */
    if(uipv4_flags & UIP_ACKDATA) {
      uipv4_connr->tcpstateflags = UIP_CLOSED;
      uipv4_flags = UIP_CLOSE;
      UIPV4_APPCALL();
    }
    break;
    
  case UIP_FIN_WAIT_1:
    /* The application has closed the connection, but the remote host
       hasn't closed its end yet. Thus we do nothing but wait for a
       FIN from the other side. */
    if(uip_len > 0) {
      uip_add_rcv_nxt(uip_len);
    }
    if(BUF->flags & TCP_FIN) {
      if(uipv4_flags & UIP_ACKDATA) {
				uipv4_connr->tcpstateflags = UIP_TIME_WAIT;
				uipv4_connr->timer = 0;
				uipv4_connr->len = 0;
      } else {
				uipv4_connr->tcpstateflags = UIP_CLOSING;
      }
      uip_add_rcv_nxt(1);
      uipv4_flags = UIP_CLOSE;
      UIPV4_APPCALL();
      goto tcp_send_ack;
    } else if(uipv4_flags & UIP_ACKDATA) {
      uipv4_connr->tcpstateflags = UIP_FIN_WAIT_2;
      uipv4_connr->len = 0;
      goto drop;
    }
    if(uip_len > 0) {
      goto tcp_send_ack;
    }
    goto drop;
      
  case UIP_FIN_WAIT_2:
    if(uip_len > 0) {
      uip_add_rcv_nxt(uip_len);
    }
    if(BUF->flags & TCP_FIN) {
      uipv4_connr->tcpstateflags = UIP_TIME_WAIT;
      uipv4_connr->timer = 0;
      uip_add_rcv_nxt(1);
      uipv4_flags = UIP_CLOSE;
      UIPV4_APPCALL();
      goto tcp_send_ack;
    }
    if(uip_len > 0) {
      goto tcp_send_ack;
    }
    goto drop;

  case UIP_TIME_WAIT:
    goto tcp_send_ack;
    
  case UIP_CLOSING:
    if(uipv4_flags & UIP_ACKDATA) {
      uipv4_connr->tcpstateflags = UIP_TIME_WAIT;
      uipv4_connr->timer = 0;
    }
  }
  goto drop;
  
  /* We jump here when we are ready to send the packet, and just want
     to set the appropriate TCP sequence numbers in the TCP header. */
 tcp_send_ack:
  BUF->flags = TCP_ACK;
  
 tcp_send_nodata:
  uip_len = UIPV4_IPTCPH_LEN;

 tcp_send_noopts:
  BUF->tcpoffset = (UIP_TCPH_LEN / 4) << 4;

  /* We're done with the input processing. We are now ready to send a
     reply. Our job is to fill in all the fields of the TCP and IP
     headers before calculating the checksum and finally send the
     packet. */
 tcp_send:
  BUF->ackno[0] = uipv4_connr->rcv_nxt[0];
  BUF->ackno[1] = uipv4_connr->rcv_nxt[1];
  BUF->ackno[2] = uipv4_connr->rcv_nxt[2];
  BUF->ackno[3] = uipv4_connr->rcv_nxt[3];
  
  BUF->seqno[0] = uipv4_connr->snd_nxt[0];
  BUF->seqno[1] = uipv4_connr->snd_nxt[1];
  BUF->seqno[2] = uipv4_connr->snd_nxt[2];
  BUF->seqno[3] = uipv4_connr->snd_nxt[3];

  BUF->proto = UIP_PROTO_TCP;
  
  BUF->srcport  = uipv4_connr->lport;
  BUF->destport = uipv4_connr->rport;

  uipv4_ipaddr_copy(&BUF->srcipaddr, &uipv4_hostaddr);
  uipv4_ipaddr_copy(&BUF->destipaddr, &uipv4_connr->ripaddr);

  if(uipv4_connr->tcpstateflags & UIP_STOPPED) {
    /* If the connection has issued uip_stop(), we advertise a zero
       window so that the remote host will stop sending data. */
    BUF->wnd[0] = BUF->wnd[1] = 0;
  } else {
    BUF->wnd[0] = ((UIP_RECEIVE_WINDOW) >> 8);
    BUF->wnd[1] = ((UIP_RECEIVE_WINDOW) & 0xff);
  }
  
 tcp_send_noconn:
  BUF->ttl = UIP_TTL;
  BUF->len[0] = (uip_len >> 8);
  BUF->len[1] = (uip_len & 0xff);

  BUF->urgp[0] = BUF->urgp[1] = 0;
  
  /* Calculate TCP checksum. */
  BUF->tcpchksum = 0;
  BUF->tcpchksum = ~(uipv4_tcpchksum());

#endif /* UIPV4_TCP */

 ip_send_nolen:
  BUF->vhl = 0x45;
  BUF->tos = 0;
  BUF->ipoffset[0] = BUF->ipoffset[1] = 0;
  ++ipid;
  BUF->ipid[0] = ipid >> 8;
  BUF->ipid[1] = ipid & 0xff;
  /* Calculate IP checksum. */
  BUF->ipchksum = 0;
  BUF->ipchksum = ~(uipv4_ipchksum());
  DEBUG_PRINTF("uip ip_send_nolen: chkecum 0x%04x\n", uipv4_ipchksum());
  UIP_STAT(++uip_stat.tcp.sent);

  DEBUG_PRINTF("Sending packet with length %d (%d)\n", uip_len,
	       (BUF->len[0] << 8) | BUF->len[1]);
  
  UIP_STAT(++uip_stat.ip.sent);
  /* Return and let the caller do the actual transmission. */
  uipv4_flags = 0;
  return;

 drop:
  uip_len = 0;
  uipv4_flags = 0;
  return;
}
/*---------------------------------------------------------------------------*/
void
uipv4_send(const void *data, int len)
{
  int copylen;
#define MIN(a,b) ((a) < (b)? (a): (b))
  copylen = MIN(len, UIP_BUFSIZE - UIP_LLH_LEN - UIPV4_TCPIP_HLEN -
		(int)((char *)uipv4_sappdata - (char *)&uip_buf[UIP_LLH_LEN + UIPV4_TCPIP_HLEN]));
  if(copylen > 0) {
    uipv4_slen = copylen;
    if(data != uipv4_sappdata) {
      memcpy(uipv4_sappdata, (data), uipv4_slen);
    }
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
