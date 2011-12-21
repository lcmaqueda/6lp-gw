#ifndef PGW_NETSTACK_H_
#define PGW_NETSTACK_H_

/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * $Id: pgw_netstack.h,v 1.6 2010/10/03 20:37:32 adamdunkels Exp $
 */

/**
 * \file
 *         Include file for the Contiki low-layer network stack (NETSTACK)
 * \author
 *         Luis Maqueda <luis@sen.se>
 */
#include "contiki-conf.h"

#include "dev/radio_driver.h"
#include "dev/eth_driver.h"
#include "net/mac/mac_eth_driver.h"
#include "net/mac/pgw_sicslowmac.h"
#include "net/p-gw/pgw_sicslowpan.h"
#include "net/p-gw/pgw.h"
#include "net/uipv4/tcpipv4.h"
#include "net/tcpip.h"
/*
 * Stack definitions. These definitions are different from those predefined with 
 * Contiki.
 */
#define NETSTACK_ETHERNET			eth_driver
#define NETSTACK_MAC_ETH			mac_eth_driver

#define NETSTACK_RADIO				radio_driver
#define NESTACK_MAC_RADIO			sicslowmac_l2gw_driver
#define NETSTACK_6LOWPAN			sicslowpan_l2gw_driver

#define NETSTACK_NETWORK_IPV4 ipv4_driver
#define NETSTACK_NETWORK_IPV6	ipv6_driver

#define NETSTACK_6LPGW				proxy_gateway_driver

/**
 * The structure of an IPv4 network driver in Contiki.
 */
struct network_ipv4_driver {
  char *name;

  /** Initialize the network driver */
  void (* init)(void);

  /** Callback for getting notified of incoming packet. */
  void (* input)(void);
  
  /** Output function */
  u8_t (* output)(void);
  
};

/**
 * The structure of an IPv6 network driver in Contiki.
 */
struct network_ipv6_driver {
  char *name;

  /** Initialize the network driver */
  void (* init)(void);

  /** Callback for getting notified of incoming packet. */
  void (* input)(void);
  
  /** Output function */
  u8_t (* output)(uip_lladdr_t*);
  
};

/**
 * The structure of a 6lowpan network driver in Contiki.
 */
struct network_6lowpan_driver {
  char *name;

  /** Initialize the network driver */
  void (* init)(void);

  /** Callback for getting notified of incoming packet. */
  void (* input)(void);
  
  /** Output function */
  u8_t (* output)(uip_lladdr_t*);
  
};

void pgw_netstack_init(void);


#endif /*PGW_NETSTACK_H_*/
