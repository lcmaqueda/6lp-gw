/**
 * \addtogroup uip
 * @{
 */

/**
 * \defgroup uipopt Configuration options for uIP
 * @{
 *
 * uIP is configured using the per-project configuration file
 * "uipopt.h". This file contains all compile-time options for uIP and
 * should be tweaked to match each specific project. The uIP
 * distribution contains a documented example "uipopt.h" that can be
 * copied and modified for each project.
 *
 * \note Contiki does not use the uipopt.h file to configure uIP, but
 * uses a per-port uip-conf.h file that should be edited instead.
 */

/**
 * \file
 * Configuration options for uIP.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 * This file is used for tweaking various configuration options for
 * uIP. You should make a copy of this file into one of your project's
 * directories instead of editing this example "uipopt.h" file that
 * comes with the uIP distribution.
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
 * $Id: uipopt.h,v 1.12 2010/10/24 22:28:43 adamdunkels Exp $
 *
 */

#ifndef __UIPV4OPT_H__
#define __UIPV4OPT_H__

#include "net/uipopt.h"
#include "contiki-conf.h"

/*------------------------------------------------------------------------------*/

/**
 * \defgroup uipoptstaticconf Static configuration options
 * @{
 *
 * These configuration options can be used for setting the IP address
 * settings statically, but only if UIPV4_FIXEDADDR is set to 1. The
 * configuration options for a specific node includes IP address,
 * netmask and default router as well as the Ethernet address. The
 * netmask, default router and Ethernet address are applicable only
 * if uIP should be run over Ethernet.
 *
 * This options are meaningful only for the IPv4 code.
 *
 * All of these should be changed to suit your project.
 */

/**
 * Determines if uIP should use a fixed IP address or not.
 *
 * If uIP should use a fixed IP address, the settings are set in the
 * uipopt.h file. If not, the macros uip_sethostaddr(),
 * uip_setdraddr() and uip_setnetmask() should be used instead.
 *
 * \hideinitializer
 */
#define UIPV4_FIXEDADDR    0

/**
 * Specifies if the uIP ARP module should be compiled with a fixed
 * Ethernet MAC address or not.
 *
 * If this configuration option is 0, the macro uip_setethaddr() can
 * be used to specify the Ethernet address at run-time.
 *
 * \hideinitializer
 */
#define UIPV4_FIXEDETHADDR 0

/** @} */
/*------------------------------------------------------------------------------*/
/**
 * \defgroup uipoptudp UDP configuration options
 * @{
 *
 * \note The UDP support in uIP is still not entirely complete; there
 * is no support for sending or receiving broadcast or multicast
 * packets, but it works well enough to support a number of vital
 * applications such as DNS queries, though
 */

/**
 * Toggles whether UDP support should be compiled in or not.
 *
 * \hideinitializer
 */
#ifdef UIPV4_CONF_UDP
#define UIPV4_UDP UIPV4_CONF_UDP
#else /* UIPV4_CONF_UDP */
#define UIPV4_UDP           1
#endif /* UIPV4_CONF_UDP */

/**
 * Toggles if UDP checksums should be used or not.
 *
 * \note Support for UDP checksums is currently not included in uIP,
 * so this option has no function.
 *
 * \hideinitializer
 */
#ifdef UIPV4_CONF_UDP_CHECKSUMS
#define UIPV4_UDP_CHECKSUMS UIPV4_CONF_UDP_CHECKSUMS
#else
#define UIPV4_UDP_CHECKSUMS UIP_CONF_IPV6
#endif

/**
 * The maximum amount of concurrent UDP connections.
 *
 * \hideinitializer
 */
#ifdef UIPV4_CONF_UDP_CONNS
#define UIPV4_UDP_CONNS UIPV4_CONF_UDP_CONNS
#else /* UIPV4_CONF_UDP_CONNS */
#define UIPV4_UDP_CONNS    10
#endif /* UIPV4_CONF_UDP_CONNS */

/**
 * The name of the function that should be called when UDP datagrams arrive.
 *
 * \hideinitializer
 */


/** @} */
/*------------------------------------------------------------------------------*/
/**
 * \defgroup uipopttcp TCP configuration options
 * @{
 */

/**
 * Toggles whether TCP support for IPv4 should be compiled in or not.
 *
 * \hideinitializer
 */
#ifdef UIPV4_CONF_TCP
#define UIPV4_TCP UIP_CONF_TCP
#else /* UIPV4_CONF_TCP */
#define UIPV4_TCP           0
#endif /* UIPV4_CONF_UDP */

/**
 * Determines if support for opening connections from uIP should be
 * compiled in.
 *
 * If the applications that are running on top of uIP for this project
 * do not need to open outgoing TCP connections, this configuration
 * option can be turned off to reduce the code size of uIP.
 *
 * \hideinitializer
 */
#ifndef UIPV4_CONF_ACTIVE_OPEN
#define UIPV4_ACTIVE_OPEN 1
#else /* UIP_CONF_ACTIVE_OPEN */
#define UIPV4_ACTIVE_OPEN UIPV4_CONF_ACTIVE_OPEN
#endif /* UIP_CONF_ACTIVE_OPEN */

/**
 * The maximum number of simultaneously open TCP connections.
 *
 * Since the TCP connections are statically allocated, turning this
 * configuration knob down results in less RAM used. Each TCP
 * connection requires approximately 30 bytes of memory.
 *
 * \hideinitializer
 */
#ifndef UIPV4_CONF_MAX_CONNECTIONS
#define UIPV4_CONNS       10
#else /* UIP_CONF_MAX_CONNECTIONS */
#define UIPV4_CONNS UIPV4_CONF_MAX_CONNECTIONS
#endif /* UIP_CONF_MAX_CONNECTIONS */


/**
 * The maximum number of simultaneously listening TCP ports.
 *
 * Each listening TCP port requires 2 bytes of memory.
 *
 * \hideinitializer
 */
#ifndef UIPV4_CONF_MAX_LISTENPORTS
#define UIPV4_LISTENPORTS 20
#else /* UIP_CONF_MAX_LISTENPORTS */
#define UIPV4_LISTENPORTS UIPV4_CONF_MAX_LISTENPORTS
#endif /* UIP_CONF_MAX_LISTENPORTS */

/**
 * Broadcast support.
 *
 * This flag configures IPv4 broadcast support. This is useful only
 * together with UDP.
 *
 * \hideinitializer
 *
 */
#ifndef UIPV4_CONF_BROADCAST
#define UIPV4_BROADCAST 0
#else /* UIP_CONF_BROADCAST */
#define UIPV4_BROADCAST UIPV4_CONF_BROADCAST
#endif /* UIP_CONF_BROADCAST */


#endif /* __UIPV4OPT_H__ */
/** @} */
/** @} */
