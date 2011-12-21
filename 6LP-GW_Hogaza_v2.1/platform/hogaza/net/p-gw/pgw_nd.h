/**
 * \file		pgw_nd.h
 *
 * \brief		ND-related definitions for the 6LoWPAN-ND proxy-gateway 
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#ifndef PGW_ND_H_
#define PGW_ND_H_

#include "contiki.h"
#include "contiki-net.h"

#define  PGW_GARBAGE_COLLECTIBLE 0
#define  PGW_TENTATIVE 1
#define  PGW_REGISTERED 2

#define UIP_ND6_OPT_ARO                 131
#if CONF_6LOWPAN_ND_6CO
#define UIP_ND6_OPT_6CO                 32
#endif /* CONF_6LOWPAN_ND_6CO */
#if CONF_OPT_ABRO
#define UIP_ND6_OPT_ABRO                33
#endif /* #if CONF_OPT_ABRO */

#define UIP_ND6_OPT_ARO_LEN     	   16

#define ARO_STATUS_SUCCESS				0
#define ARO_STATUS_DUPLICATE			1
#define ARO_STATUS_RTR_NC_FULL			2

#define UIP_ND6_RA_FLAG_COMPRESSION     0x10
#define UIP_ND6_RA_CID						      0x0F


typedef struct uip_nd6_opt_aro {
  u8_t type;
  u8_t len;
  u8_t status;
  u8_t reserved1;
  u16_t reserved2;
  u16_t lifetime;
  uip_lladdr_t eui64;
} uip_nd6_opt_aro ;

typedef struct uip_nd6_opt_6co {
  u8_t type;
  u8_t len;
  u8_t preflen;
  u8_t res1_c_cid;
  u16_t reserved2;
  u16_t lifetime;
  uip_ipaddr_t prefix;
} uip_nd6_opt_6co ;

/** \brief An entry in the 6LP-GW nbr cache */
typedef struct pgw_nbr {
  u8_t isused;
  uip_ipaddr_t ipaddr;
  eui64_t lladdr;
  struct stimer reachable;
  clock_time_t last_lookup;
  u8_t isrouter;
  u8_t state;
  /* Extra variables required for 6LP-GW operation */
  u8_t aro_pending;
  u8_t ra_pending;
  struct timer dadtimer;
  u8_t dadnscount;
} pgw_nbr_t;

typedef enum pgw_context_state {
	NOT_IN_USE = 0,
	IN_USE_UNCOMPRESS_ONLY,
	IN_USE_COMPRESS,
	EXPIRED,
} pgw_context_state_t;
 
typedef struct pgw_addr_context {
  pgw_context_state_t state;
  u8_t length;
  u8_t context_id;
  uip_ipaddr_t prefix;
  struct stimer vlifetime;
  /* According to I-D.ietf-6lowpan-nd, if a context valid lifetime expires,
   * it must be set to a only-decompression state during a period of "twice
   * the Default Router Lifetime". After that period, if no 6CO has been 
   * received to update that context, it should be deleted. Therefore, we 
   * need to keep the default router lifetime. */
  //u16_t defrt_lifetime; Not needed in 6LP-GW/6LBR
} pgw_addr_context_t;

/* External variables */

/** \brief Timer for maintenance of data structures */
extern struct etimer pgw_timer_periodic;
/** \brief The Neighbor cache for the 6LoWPAN segment*/
extern pgw_nbr_t pgw_6ln_cache[MAX_6LOWPAN_NEIGHBORS];
/** \brief The Context Table */
extern pgw_addr_context_t pgw_addr_context_table[PGW_CONF_MAX_ADDR_CONTEXTS];

/* External Functions */

void pgw_nd_init();
pgw_nbr_t* pgw_nbr_lookup(uip_ipaddr_t *ipaddr);
void pgw_nbr_rm(pgw_nbr_t *nbr);
pgw_nbr_t* pgw_nbr_add(uip_ipaddr_t * ipaddr, uip_lladdr_t * lladdr,
												u8_t isrouter, u8_t state);
pgw_addr_context_t* pgw_context_add(uip_nd6_opt_6co *context_option, u16_t defrt_lifetime);
pgw_addr_context_t* pgw_context_create(uip_ipaddr_t *prefix, u8_t length);
void pgw_context_rm(pgw_addr_context_t *context);
pgw_addr_context_t* pgw_context_lookup_by_id(u8_t context_id);
pgw_addr_context_t* pgw_context_lookup_by_prefix(uip_ipaddr_t *prefix);
void pgw_periodic();
#endif /*PGW_6LOWPANND_H_*/
