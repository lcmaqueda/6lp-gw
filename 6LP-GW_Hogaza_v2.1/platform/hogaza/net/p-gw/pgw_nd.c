/**
 * \file		pgw_nd.c
 *
 * \brief		ND-related functions for the 6LoWPAN-ND proxy-gateway 
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#include "contiki.h"
#include "net/p-gw/pgw_nd.h"
#include "net/p-gw/pgw.h"
#include "net/p-gw/pgw_fwd.h"
#include "contiki-net.h"
#include "net/uip-nd6.h"

static pgw_addr_context_t *loccontext;			/** \brief Pointer to a context */
static u8_t context_id;											/** \brief Context index */
static pgw_nbr_t *locnbr;										/** \brief Pointer to a NCE */
static uip_ipaddr_t *locipaddr; 						/** \brief Pointer to an ipv6 address */
static uip_ipaddr_t fipaddr;								/** \brief A full ipv6 address (in contrast to a pointer)*/
/** \brief Timer for maintenance of data structures */
struct etimer pgw_timer_periodic;
/** \brief The Neighbor cache for the 6LoWPAN segment*/
pgw_nbr_t pgw_6ln_cache[MAX_6LOWPAN_NEIGHBORS];
/** \brief The Context Table */
pgw_addr_context_t pgw_addr_context_table[PGW_CONF_MAX_ADDR_CONTEXTS];


/* Function prototypes */

void pgw_dad(pgw_nbr_t* nbr);
void pgw_dad_failed(pgw_nbr_t* nbr);
void pgw_dad_response(pgw_nbr_t* nbr, u8_t status);

void
pgw_nd_init()
{
	memset(pgw_6ln_cache, 0, sizeof(pgw_6ln_cache));
	memset(pgw_addr_context_table, 0, sizeof(pgw_addr_context_table));
	etimer_set(&pgw_timer_periodic, PGW_PERIOD);
}

/*---------------------------------------------------------------------------*/
pgw_nbr_t*
pgw_nbr_lookup(uip_ipaddr_t *ipaddr)
{
  if(uip_ds6_list_loop
     ((uip_ds6_element_t *) pgw_6ln_cache, UIP_DS6_NBR_NB,
      sizeof(pgw_nbr_t), ipaddr, 128,
      (uip_ds6_element_t **) & locnbr) == FOUND) {
    return locnbr;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
pgw_nbr_rm(pgw_nbr_t *nbr)
{
	if(nbr != NULL) {
   	nbr->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
pgw_nbr_t*
pgw_nbr_add(uip_ipaddr_t * ipaddr, uip_lladdr_t * lladdr,
                u8_t isrouter, u8_t state)
{
  int r;

  r = uip_ds6_list_loop((uip_ds6_element_t *)pgw_6ln_cache, UIP_DS6_NBR_NB,
      									sizeof(pgw_nbr_t), ipaddr, 128, (uip_ds6_element_t **)&locnbr);

  if(r == FREESPACE) {
    locnbr->isused = 1;
    uip_ipaddr_copy(&(locnbr->ipaddr), ipaddr);
    if(lladdr != NULL) {
      memcpy(&(locnbr->lladdr), lladdr, UIP_LLADDR_LEN);
    } else {
      memset(&(locnbr->lladdr), 0, UIP_LLADDR_LEN);
    }
    locnbr->isrouter = isrouter;
    locnbr->state = state;
		if(locnbr->state == PGW_GARBAGE_COLLECTIBLE) {
			stimer_set(&(locnbr->reachable), GARBAGE_COLLECTIBLE_NCE_LIFETIME);
		} else if (locnbr->state == PGW_TENTATIVE){
			stimer_set(&(locnbr->reachable), TENTATIVE_NCE_LIFETIME);
		}
		locnbr->aro_pending = 0;
		locnbr->ra_pending = 0;
    locnbr->last_lookup = clock_time();
    return locnbr;
  } else if(r == NOSPACE) {
    /* We did not find any empty slot on the neighbor list, so we need
       to remove one old entry to make room. */
    pgw_nbr_t *n, *oldest;
    clock_time_t oldest_time;

    oldest = NULL;
    oldest_time = clock_time();

    for(n = pgw_6ln_cache; n < &pgw_6ln_cache[UIP_DS6_NBR_NB]; n++) {
      if(n->isused) {
        if((n->last_lookup < oldest_time) && (n->state == PGW_GARBAGE_COLLECTIBLE)) {
        	/* We do not want to remove any registered or tentative entry */
          oldest = n;
          oldest_time = n->last_lookup;
        }
      }
    }
    if(oldest != NULL) {
      pgw_nbr_rm(oldest);
      return pgw_nbr_add(ipaddr, lladdr, isrouter, state);
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/

pgw_addr_context_t*
pgw_context_add(uip_nd6_opt_6co *context_option, u16_t defrt_lifetime) {
	
	for(loccontext = pgw_addr_context_table;
      loccontext < pgw_addr_context_table + SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS; loccontext++) {
    if(loccontext->state == NOT_IN_USE) {
    	loccontext->length = context_option->len;
    	loccontext->context_id = context_option->res1_c_cid & UIP_ND6_RA_CID;
    	uip_ipaddr_copy(&loccontext->prefix, &context_option->prefix);
    	if (context_option->res1_c_cid & UIP_ND6_RA_FLAG_COMPRESSION) {
	    	loccontext->state = IN_USE_COMPRESS;
    	} else {
    		loccontext->state = IN_USE_UNCOMPRESS_ONLY;
    	}
    	return loccontext;
    }
  }
	return NULL;
}

pgw_addr_context_t*
pgw_context_create(uip_ipaddr_t *prefix, u8_t length) {

	for (context_id = 0; context_id < PGW_CONF_MAX_ADDR_CONTEXTS; context_id++) {
		if (pgw_addr_context_table[context_id].state == NOT_IN_USE) {
			pgw_addr_context_table[context_id].length = length;
    	pgw_addr_context_table[context_id].context_id = context_id;
    	uip_ipaddr_copy(&pgw_addr_context_table[context_id].prefix, prefix);
			/* Contexts are always created in IN_USE_UNCOMPRESS_ONLY state */
//    	pgw_addr_context_table[context_id].state = IN_USE_COMPRESS;
    	pgw_addr_context_table[context_id].state = IN_USE_UNCOMPRESS_ONLY;
    	stimer_set(&pgw_addr_context_table[context_id].vlifetime, 
    							PGW_INITIAL_CONTEXT_LIFETIME); 
    	return &pgw_addr_context_table[context_id];
		}
	}
	return NULL;
} 

void 
pgw_context_rm(pgw_addr_context_t *context){
	context->state = NOT_IN_USE;
}

pgw_addr_context_t *
pgw_context_lookup_by_id(u8_t context_id){

	if (pgw_addr_context_table[context_id].state != NOT_IN_USE) {
		return &pgw_addr_context_table[context_id];
	} else {
  	return NULL;
	}
}

pgw_addr_context_t*
pgw_context_lookup_by_prefix(uip_ipaddr_t *prefix) {
	
	for(loccontext = pgw_addr_context_table;
      loccontext < pgw_addr_context_table + SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS; loccontext++) {
    if(loccontext->state != NOT_IN_USE) {
			if (uip_ipaddr_prefixcmp(prefix, &loccontext->prefix, loccontext->length)) {
				return loccontext;
			}
    }
  }
  return NULL;
}

/**
 * \brief 	Performs periodic tasks for 6LP-GW variable management, such as
 * 			processing of neighbor lifetimes and DAD timers
 */ 
void
pgw_periodic() 
{
	
	/* periodic processing of contexts */
	for(loccontext = pgw_addr_context_table;
      loccontext < pgw_addr_context_table + SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS; loccontext++) {
  	if (loccontext->state != NOT_IN_USE) {
  		if (stimer_expired(&loccontext->vlifetime)) {
				switch(loccontext->state) {
				case IN_USE_UNCOMPRESS_ONLY:
					loccontext->state = IN_USE_COMPRESS;
					stimer_set(&loccontext->vlifetime, PGW_CONTEXT_LIFETIME);
					context_chaged = 1;
					break;
				case IN_USE_COMPRESS:
					loccontext->state = EXPIRED;
					if (loccontext->vlifetime.interval > PGW_MIN_CONTEXT_CHANGE_DELAY) {
						stimer_reset(&loccontext->vlifetime);
					} else {
						/* This way we make sure that, if the context is eventually deleted, 
						 * No other context will use its id in a period of at least 
						 * MIN_CONTEXT_CHANGE_DELAY */
						stimer_set(&loccontext->vlifetime, PGW_MIN_CONTEXT_CHANGE_DELAY);
					}
					context_chaged = 1;
					break;
				case EXPIRED:
					pgw_context_rm(loccontext);
					break;
				}  			
  		}
    }
	}
	
	/* periodic processing of neighbors */
	for(locnbr = pgw_6ln_cache; locnbr < pgw_6ln_cache + MAX_6LOWPAN_NEIGHBORS; locnbr++) {
		if(locnbr->isused) {
			/* 
			 * If the reachable timer is expired, we delete the NCE, 
			 * regardless of the NCE's state.
			 */
			if(stimer_expired(&(locnbr->reachable))) {
				/* I-D.ietf-6lowpan-nd: Should the Registration Lifetime in a NCE expire,
				 * then the router MUST delete the cache entry. */
				pgw_nbr_rm(locnbr);
			} else if ((locnbr->state == PGW_TENTATIVE) && 
					(!uip_is_addr_link_local(&locnbr->ipaddr))) {
				if ((locnbr->dadnscount <= PGW_MAX_DAD_NS) &&
						(timer_expired(&locnbr->dadtimer))) {
        	pgw_dad(locnbr);
        	/* If we found a neighbor requiring DAD, perform it. If there were 
        	 * more neighbors requiring it, we'll do it in further invocations */
       	 	return;
				}	
      }
    }
	}
	
	etimer_reset(&pgw_timer_periodic);
	return;
}

/*
 * Perform DAD on behalf of a 6LN
 */
void 
pgw_dad(pgw_nbr_t* nbr)
{
	/* send maxdadns NS for DAD  */
  if(nbr->dadnscount < PGW_MAX_DAD_NS) {
  	/* 
  	 * The packet is sent on behalf of a 6LN, so its incoming_if is IEEE_802_15_4.
  	 * Even though it is a multicast packet, its outgoing_if must be IEEE_802_3.
  	 */
  	incoming_if = IEEE_802_15_4;
  	outgoing_if = IEEE_802_3;
  	
  	/* Set src and dst MAC addresses */
		eui64_copy(&src_eui64, &nbr->lladdr);
		eui64_copy(&dst_eui64, &rimeaddr_null);
  		
	  pgw_create_ns(NULL, NULL, &nbr->ipaddr);
	  /* No options in DAD NS */
	  pgw_update_icmp_checksum();
  	nbr->dadnscount++;
   	timer_set(&nbr->dadtimer, UIP_ND6_RETRANS_TIMER / 1000 * CLOCK_SECOND);
   	return;
  }
  /*
   * If we arrive here it means DAD succeeded, otherwise the dad process
   * would have been interrupted in ns/na_input
   */
  nbr->state = PGW_REGISTERED;
  nbr->aro_pending = 0;
  pgw_dad_response(nbr, ARO_STATUS_SUCCESS);
  return;
}

void 
pgw_dad_failed(pgw_nbr_t* nbr)
{
	/* 
	 * The node who sent the NS with ARO is awaiting for response 
	 * so let's respond it.
	 */
	pgw_dad_response(nbr, ARO_STATUS_DUPLICATE);
	/* And the delete the NCE */
	pgw_nbr_rm(nbr);
}

/* This function generates a NA with ARO to be sent to the 6LN corresponding 
 * to nbr.  It makes no assumtions regarding the contents of uip_buf. 
 */
void
pgw_dad_response(pgw_nbr_t* nbr, u8_t status) 
{
	
	if (status == ARO_STATUS_SUCCESS) {
		locipaddr = &nbr->ipaddr;
	} else {
		/* Address registration errors are not sent back to the source address
     * of the NS due to a possible risk of L2 address collision. Instead
     * the NA is sent to the link-local IPv6 address with the IID part
     * derived from the EUI-64 field of the ARO (I-D.ietf-6lowpan-nd). 
     */
		create_eui64_based_ipaddr(&fipaddr, &nbr->lladdr);
		locipaddr = &fipaddr;
	}
	/* Create NA on behalf of the router*/    		
	pgw_create_na(&rr_ipaddr, locipaddr, &rr_ipaddr, 
					UIP_ND6_NA_FLAG_ROUTER | 
					UIP_ND6_NA_FLAG_SOLICITED | 
					UIP_ND6_NA_FLAG_OVERRIDE);
	/* include TLLAO option */
	pgw_append_icmp_opt(UIP_ND6_OPT_TLLAO, &rr_lladdr, 0, 0);
	/* include ARO option */
	pgw_append_icmp_opt(UIP_ND6_OPT_ARO, &nbr->lladdr, 
						status, uip_htons(nbr->reachable.interval / 60));
	/* Compute checksum */
	pgw_update_icmp_checksum();
	/* Set outgoing interface and src and dst MAC addresses */
	outgoing_if = IEEE_802_15_4;
	/* As this packet did not come from anywhere, set the incoming_if */
	incoming_if = IEEE_802_15_4;
	eui64_copy(&src_eui64, &rr_lladdr);
	eui64_copy(&dst_eui64, &nbr->lladdr);
}

