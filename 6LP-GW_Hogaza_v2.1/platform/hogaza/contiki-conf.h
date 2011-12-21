/**
 * \file		contiki_conf.h
 *
 * \brief		Platform-specific constant definitions.
 *
 * \author		Luis Maqueda <luis@sen.se>
 */
#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

/* Compiler specific includes */
#include <msp430f5435a.h>

#define CCIF
#define CLIF

#define AUTOSTART_ENABLE 1
#define CC_CONF_REGISTER_ARGS 1
#define CC_CONF_FUNCTION_POINTER_ARGS 1
#define CC_CONF_INLINE inline

/**
 * 8 bit datatype
 *
 * This typedef defines the 8-bit type used throughout contiki.
 */
typedef unsigned char u8_t;

/**
 * 16 bit datatype
 *
 * This typedef defines the 16-bit type used throughout contiki.
 */
typedef unsigned int u16_t;

/**
 * 32 bit datatype
 *
 * This typedef defines the 32-bit type used throughout contiki.
 */
typedef unsigned long u32_t;

/**
 * 8 bit datatype
 *
 * This typedef defines the signed 8-bit type used throughout contiki.
 */
typedef signed char s8_t;

/**
 * 16 bit datatype
 *
 * This typedef defines the signed 16-bit type used throughout contiki.
 */
typedef int s16_t;

/**
 * 32 bit datatype
 *
 * This typedef defines the signed 32-bit type used throughout contiki.
 */
typedef signed long s32_t;

typedef u32_t clock_time_t;

#include "clock_arch.h"

/*
 * UIP SECTION
 */

/**
 * Statistics datatype
 *
 * This typedef defines the dataype used for keeping statistics in
 * uIP.
 */
typedef unsigned int uip_stats_t;

/*
 * We will use rime addresses to take advantage of the address handling 
 * functions provided by rime.
 */
#define RIMEADDR_CONF_SIZE              8

/* 
 * Link layer is IEEE 802.15.4
 */
#define UIP_CONF_LL_802154              1


#define UIP_CONF_IPV6                   1
#define UIP_CONF_IPV6_QUEUE_PKT         0
#define UIP_CONF_IPV6_CHECKS            1
#define UIP_CONF_IPV6_REASSEMBLY        0
#define UIP_CONF_NETIF_MAX_ADDRESSES    3
#define UIP_CONF_ND6_MAX_PREFIXES       3
#define UIP_CONF_ND6_MAX_NEIGHBORS      10
#define UIP_CONF_ND6_MAX_DEFROUTERS     1
#define UIP_CONF_IP_FORWARD             0

/* 
 * Define the Ethernet header length
 */
#define UIP_CONF_LLH_LEN						14

/* IPv6 minumum MTU is 1280 bytes. Ethernet header (14) is placed on the same
 * buffer, which defines our maximum buffer size as 1280 + 14 */
#define UIP_CONF_BUFFER_SIZE					1280 + 14

/* UIP_CONF_ICMP6 enables the use of application level events, callbacks
 * and app polling in case ICMPv6 packet arrives. */
#define UIP_CONF_ICMP6							0

/* 
 * Enable IPv6 UDP support.
 */
#define UIP_CONF_UDP							1

/* 
 * Enable IPv4 UDP support.
 */
#define UIPV4_CONF_UDP							1

/*
 * Enable multicast UPD support for IPv4 (needed for DHCP) 
 */
#define UIPV4_CONF_BROADCAST					1

/* Enable UDP Checksums for IPv6. If 0 they are not sent in the packet nor
 * verified at packet arrival. Note that they are mandatory by the IPv6
 * standard. */
#define UIP_CONF_UDP_CHECKSUMS					1

/* Enable UDP Checksums for IPv4. If 0 they are not sent in the packet nor
 * verified at packet arrival. Note that, as opposed to IPv6, they are NOT
 * mandatory by the IPv4 standard. */
#define UIPV4_CONF_UDP_CHECKSUMS				1

/*
 * TCP support in the IPv6 stack
 */
#define UIP_CONF_TCP							0

/*
 * TCP support in the IPv4 stack
 */
#define UIPV4_CONF_TCP							1

/* 
 * Override uip_add32 (32-bit addition)
 */
#define UIP_ARCH_ADD32 							1

/*
 * Override Contiki's checksum mechanisms by architecture-specific ones
 */
#define UIP_ARCH_CHKSUM 						1

/*
 * Override Contiki's IP-level checksum mechanism by architecture-specific ones
 */
#define UIP_ARCH_IPCHKSUM 						1

/* The 3 different compression methods supported by contiki. Note that
 * SICSLOWPAN_CONF_COMPRESSION_IPV6 means no compression, only the dispatch byte
 * is prepended and then the IPv6 packet is sent inline.
 *
 * #define SICSLOWPAN_COMPRESSION_IPV6        0
 * #define SICSLOWPAN_COMPRESSION_HC1         1
 * #define SICSLOWPAN_COMPRESSION_HC06        2
 */

/* Now we define which one from the above compression methods we are going to
 * use. */
#define SICSLOWPAN_CONF_COMPRESSION           	SICSLOWPAN_COMPRESSION_HC06

/*
 * We define the maximum number of context to use in 6LoWPAN IPHC
 */
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS		2

/*
 * 6LoWPAN fragmentation support
 */
#define SICSLOWPAN_CONF_FRAG                    0

/*
 * Does the local-host behave as router?
 */
#define UIP_CONF_ROUTER							0

/*
 * Does the local-host use a manually-assigned, fixed address?
 */
#define UIP_FIXEDADDR							0

/*
 * Node's MAC address
 */
#define NODE_BASE_ADDR0	0x00
#define NODE_BASE_ADDR1 0x07
#define NODE_BASE_ADDR2 0x62
#define NODE_BASE_ADDR3 0xff
#define NODE_BASE_ADDR4 0xfe
 
/*
 * Do optional option filtering?
 */
#define CONF_OPT_FILTERING 	1

/*
 * Use 6LoWPAN-ND IN HOST? (We use instead traditional IPv6-ND)
 */
#define CONF_6LOWPAN_ND		0

/*
 * Do we implement the 6LoWPAN Context option?
 */
#define CONF_6LOWPAN_ND_6CO		1

/*
 * Do we implement the 6LoWPAN Authoritative Border Router option?
 */
#define CONF_OPT_ABRO		0

/*
 * State-dependent NCEs' lifetime definitions
 */
#define GARBAGE_COLLECTIBLE_NCE_LIFETIME	600 /* 10 minutes */
#define TENTATIVE_NCE_LIFETIME				20 /* 20 seconds */

/*
 * 6LP-GW definitions
 */

/*
 * Maximum number of allowed neighbors
 */
#define MAX_6LOWPAN_NEIGHBORS	25


#endif /* CONTIKI_CONF_H */
