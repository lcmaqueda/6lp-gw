/**
 * \file			pgw.c
 *
 * \brief			Set of functions needed for the 6LP-GW operation 
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#include "contiki.h"
#include "net/p-gw/pgw.h"
#include "net/p-gw/pgw_nd.h"
#include "net/p-gw/pgw_fwd.h"
#include "contiki-net.h"
#include "net/uip-icmp6.h"
#include "net/uip-nd6.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define ETH_BUF 	((struct uip_eth_hdr *)&uip_buf[0])
#define UIP_ICMP_BUF     ((struct uip_icmp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_ICMP_OPTS_APPEND ((uip_nd6_opt_hdr *)&uip_buf[UIP_LLH_LEN + uip_len])
#define UIP_ND6_RS_BUF            ((uip_nd6_rs *)&uip_buf[uip_l2_l3_icmp_hdr_len])
#define UIP_ND6_RA_BUF            ((uip_nd6_ra *)&uip_buf[uip_l2_l3_icmp_hdr_len])
#define UIP_ND6_NS_BUF            ((uip_nd6_ns *)&uip_buf[uip_l2_l3_icmp_hdr_len])
#define UIP_ND6_NA_BUF            ((uip_nd6_na *)&uip_buf[uip_l2_l3_icmp_hdr_len])
#define UIP_PGW_OPT_HDR_BUF  ((uip_nd6_opt_hdr *)&uip_buf[uip_l2_l3_icmp_hdr_len + pgw_opt_offset])
#define CURRENT_OPT_LENGTH		(UIP_PGW_OPT_HDR_BUF->len << 3)

/* Incoming and outgoing interfaces. The incoming interface, "incoming_if" is set 
 * by the link-layer drivers (radio_driver or eth_driver) whilst the outgoing 
 * interface is set by upper layers */
uip_ipaddr_t rr_ipaddr;											/** \briefIPv6 regular router's IPv6 address */

eui64_t rr_lladdr;														/** \briefIPv6 regular router's EUI-64 address */

u8_t ra_pending = 0;					/** Global flag indicating whether any node is awaiting for a RA*/
u8_t context_chaged = 0; 			/** Global flag indicating whether there has been a change in a context */ 
static u8_t pgw_opt_offset;  			/** Offset from the end of the icmpv6 header to the option in uip_buf*/
static pgw_nbr_t *nbr;								/** \brief Pointer to a NCE */
static pgw_addr_context_t *context;									/** \brief Pointers to a context */
static uip_ipaddr_t *ipaddr; 												/** \brief Pointer to an ipv6 address */
static uip_ipaddr_t fipaddr; 												/** \brief A full ipv6 address (in contrast to a pointer)*/
static eui64_t aux_eui64;														/** \brief Auxiliar MAC addresses */	
static u8_t *pgw_opt_llao;   										/**  Pointer to llao option in uip_buf */
static uip_nd6_opt_prefix_info *pgw_opt_prefix_info;/**  Pointer to PIO option in uip_buf */
static uip_nd6_opt_aro *pgw_opt_aro;   							/**  Pointer to aro option in uip_buf */
static u16_t pgw_len = 0;
static uip_buf_t pgw_aligned_buf;

#define pgw_buf (pgw_aligned_buf.u8)


/* 
 * Local function prototypes 
 */

/* Proxy functions */
static void proxy_input(void);
static void proxy_ns_input(void);
static void proxy_na_input(void);
static void proxy_rs_input(void);
static void proxy_ra_input(void);
static void proxy_redirect_input(void);
static void pgw_registration_error(u8_t status);

/* 6LP-GW process functions */
void pgw_init(void);
void pgw_input(void);
void local_node_output(uip_lladdr_t *localdest);
static void pgw_packet_input(void);
static void pgw_output(void);
static void pgw_eventhandler(process_event_t ev, process_data_t data);

/* Support functions */
void pgw_create_ns(uip_ipaddr_t* src, uip_ipaddr_t* dst, uip_ipaddr_t* tgt);
void pgw_create_na(uip_ipaddr_t* src, uip_ipaddr_t* dst, uip_ipaddr_t* tgt, u8_t flags);
void pgw_append_icmp_opt(u8_t type, void* data, u8_t status, u16_t lifetime);
void pgw_update_icmp_checksum(void);

PROCESS(pgw_process, "6LP-GW process");

/* 
 * Initialize the 6LP-GW data structures.
 */
void 
pgw_init() 
{
	pgw_nd_init();
	pgw_fwd_init();
	process_start(&pgw_process, NULL);
}

static void 
pgw_packet_input()
{
	/* pgw_fwd_input() performs all layer-2 operations which are required 
	 * for packet forwarding: refresh bridge data filtering the packet out 
	 * if pertinent. 
	 * In addition it fills all the needed layer-2 data: src and dest 
	 * addresses as well as incoming and outgoing interfaces. */ 
	pgw_fwd_input();
	
	/* If the packet has been filtered out, uip_len is 0 */
	if (uip_len == 0) {
		return;
	}
	
	if (incoming_if == outgoing_if) {		
		goto discard;
	} else if (UIP_IP_BUF->proto == UIP_PROTO_ICMP6) {
		/* Now, depending on the values of both, incoming_if and outgoing_if, 
		 * proxy, forward or do both */
		if (((incoming_if == IEEE_802_3) 		&& (outgoing_if == IEEE_802_15_4)) ||
				((incoming_if == IEEE_802_15_4) && (outgoing_if == IEEE_802_3)) ||
				((incoming_if == LOCAL) 				&& (outgoing_if == IEEE_802_15_4)) || 
				((incoming_if == IEEE_802_15_4) && (outgoing_if == LOCAL))) {
			proxy_input();
		} else if (((incoming_if == IEEE_802_3) && (outgoing_if == LOCAL)) ||
							((incoming_if == LOCAL) 			&& (outgoing_if == IEEE_802_3))) {
			/* Forward */
			return;
		} else if (outgoing_if == UNDEFINED) {
			/* Do both (Since the proxy operation may end up sending out a different packet,
			 * we need to keep a copy uip_buf and uip_len). Note that reverting the order
			 * does not eliminates the need of a copy either, since the local node may also
			 * overwrite uip_buf if it generates a packet in response.
			 */
			pgw_len = uip_len;
			memcpy(pgw_buf, uip_buf, UIP_LLH_LEN + uip_len);
			
			if (incoming_if == LOCAL) {
				outgoing_if = IEEE_802_15_4;
				proxy_input();
				pgw_output();
				/* Restore */
				memcpy(uip_buf, pgw_buf, UIP_LLH_LEN + pgw_len);
				uip_len = pgw_len;
				outgoing_if = IEEE_802_3;
			} else if (incoming_if == IEEE_802_3) {
				outgoing_if = IEEE_802_15_4;
				proxy_input();
				pgw_output();
				/* Restore */
				memcpy(uip_buf, pgw_buf, UIP_LLH_LEN + pgw_len);
				uip_len = pgw_len;
				outgoing_if = LOCAL;
			} else if (incoming_if == IEEE_802_15_4) {
				/* In this particular case, proxy twice!*/
				outgoing_if = IEEE_802_3;
				proxy_input();
				pgw_output();
				/* Restore */
				memcpy(uip_buf, pgw_buf, UIP_LLH_LEN + pgw_len);
				uip_len = pgw_len;
				outgoing_if = LOCAL;
				proxy_input();
			} else {
				/* This should never happen */
				goto discard;
			}
		} else {
			/* This should never happen */
			goto discard;
		}
	}
	return;
discard:
	uip_len = 0;
	return;
}

static void 
proxy_input() 
{
	
	switch(UIP_ICMP_BUF->type) {
  case ICMP6_NS:
  	proxy_ns_input();
  	break;
  case ICMP6_NA:
    proxy_na_input();
    break;
  case ICMP6_RS:
  	proxy_rs_input();
		break;
  case ICMP6_RA:
  	proxy_ra_input();
		break;
	case ICMP6_REDIRECT:
		proxy_redirect_input();
		break;
//	case ICMP6_ECHO_REQUEST:
//		_nop();
//		break;
//	case ICMP6_ECHO_REPLY:
//		_nop();
//		break;	
	default:
		break;
	}		
}

static void
proxy_ns_input() 
{
	
	switch (incoming_if) {
		
	case LOCAL:
	case IEEE_802_3:
		/* 
		 * Note that, we chose to let the 6LP-GW respond to NS on 
		 * behalf of 6LoWPAN nodes to alleviate their tasks. Therefore, 
		 * all NA packets originated as response to a NS coming from the 
		 * IEEE 802.15.4 (or from the local host) will be sent through 
		 * the same interface they they came */
		outgoing_if = incoming_if;
		
		for(nbr = pgw_6ln_cache; nbr < pgw_6ln_cache + MAX_6LOWPAN_NEIGHBORS; nbr++) {
			if (nbr->isused) {
				if (uip_is_addr_linklocal((uip_ipaddr_t *)(&UIP_ND6_NS_BUF->tgtipaddr))) {
					create_eui64_based_ipaddr(&fipaddr, &nbr->lladdr);
					ipaddr = &fipaddr;
				} else {
					ipaddr = &nbr->ipaddr;
				}
				if(uip_ipaddr_cmp(ipaddr, (uip_ipaddr_t *)(&UIP_ND6_NS_BUF->tgtipaddr))) {
					/* We'll probably have to reply this packet */
					if (nbr->state == PGW_REGISTERED) {
						if (!uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
							/* message sent for AR. Respond on behalf of 6LN */
							pgw_create_na(ipaddr, &UIP_IP_BUF->srcipaddr, &UIP_ND6_NS_BUF->tgtipaddr,
														UIP_ND6_NA_FLAG_OVERRIDE | UIP_ND6_NA_FLAG_SOLICITED);
							/* Set dst. MAC address */
							eui64_copy(&dst_eui64, &src_eui64);
						} else {
							/* 
							 * Message sent for DAD. There is a IEEE 802.3 node trying to 
							 * configure an address that is in use. Inform the IEEE 802.3 node
							 * that the address is in use 
							 */
						 	uip_create_linklocal_allnodes_mcast(&fipaddr);
							pgw_create_na(ipaddr, &fipaddr, &UIP_ND6_NS_BUF->tgtipaddr,
														UIP_ND6_NA_FLAG_OVERRIDE);
							/* Set dst. MAC address */
							eui64_copy(&dst_eui64, &rimeaddr_null);
						}
						/* include TLLAO option */
						pgw_append_icmp_opt(UIP_ND6_OPT_TLLAO, &nbr->lladdr, 0, 0);
						/* Compute checksum */
						pgw_update_icmp_checksum();
						/* Set src MAC address */
						eui64_copy(&src_eui64, &nbr->lladdr);
					} else if (nbr->state == PGW_TENTATIVE) {
						if (uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
							/* 
							 * message sent for DAD. There is a IEEE 802.3 node trying to 
							 * configure an address that is duplicated in the IEEE 802.15.4 
							 * segment. DAD failed and the tentative address should not be used. 
							 */
							pgw_dad_failed(nbr);
						} else {
							/* message sent for AR. As the address is in TENTATIVE state, discard */
							goto discard;
						}
					} else {
						goto discard;
					}
					/* There can be 1 matching NCE at most. If we've found it, break the loop! */
					break;
				} 
			}
		}
		if (ipaddr == NULL) {
			/* We did not find the destination */
			goto discard;
		}
		break;
		
	case IEEE_802_15_4:
		/* ND check */
		if (uip_is_addr_mcast(&UIP_ND6_NS_BUF->tgtipaddr) || 
			(uip_is_addr_unspecified(&UIP_ND6_NS_BUF->tgtipaddr))){
			goto discard;
		}
		
		/* If the destination is a multicast address, forward the packet unchanged */
		if (uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
			goto forward;
		}
		
		/* Destination is a unicast address; retrieve SLLAO and ARO options */
		pgw_opt_llao = NULL;
		pgw_opt_aro = NULL;
		pgw_opt_offset = UIP_ND6_NS_LEN;
  	while(uip_l3_icmp_hdr_len + pgw_opt_offset < uip_len) {
			if(UIP_PGW_OPT_HDR_BUF->len == 0) {
				goto discard;
    	}
    	switch (UIP_PGW_OPT_HDR_BUF->type) {
    	case UIP_ND6_OPT_SLLAO:
      	pgw_opt_llao = &uip_buf[uip_l2_l3_icmp_hdr_len + pgw_opt_offset];
      	break;
      case UIP_ND6_OPT_ARO:
      	pgw_opt_aro = (uip_nd6_opt_aro*)&uip_buf[uip_l2_l3_icmp_hdr_len + pgw_opt_offset];
      	break;
    	}
    	pgw_opt_offset += CURRENT_OPT_LENGTH;
  	}
  	/* 
  	 * In addition to the normal validation of a Neighbor Solicitation and
  	 * its options, the Address Registration option (if present) is verified 
  	 * as follows. If the Length field is not two, or if the Status field is 
  	 * not zero, then the Neighbor Solicitation is silently ignored.
  	 * If the source address of the NS is the unspecified address, or if no
  	 * SLLA option is included, then any included ARO is ignored, that is,
  	 * the NS is processed as if it did not contain an ARO. 
  	 */
		if ((uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) || (pgw_opt_llao == NULL)) {
  		pgw_opt_aro = NULL;
  	}
  	if ((pgw_opt_aro != NULL) && 
  			((pgw_opt_aro->len != 2) || (pgw_opt_aro->status != ARO_STATUS_SUCCESS))) {
  		/* BAD ARO */
  		goto discard;			
  	}
  	/* If there is no ARO option the packet has been sent for NUD and 
  	 * therefore it must be forwarded unchanged.
  	 */
  	if (pgw_opt_aro == NULL) {
  		goto forward;
  	}
		/* 
		 * At this point, we have a unicast NS which includes ARO and SLLAO options. 
		 * This packet has therefore been sent for (re-)registration.
		 * Check first whether the destination and target addresses are the RR's address.
		 */
		if ((!uip_ipaddr_cmp(&rr_ipaddr, &UIP_IP_BUF->destipaddr)) || 
				(!uip_ipaddr_cmp(&rr_ipaddr, &UIP_ND6_NS_BUF->tgtipaddr)) ||
				(!eui64_cmp(&dst_eui64, &rr_lladdr))) {
			/* 
			 * If the destination or target IPv6 addresses in the message are not the 
			 * IPv6 address of the RR, the packet must be discarded. 
			 */
			goto discard;
		}
		/* Check if the NCE exists */
		nbr = pgw_nbr_lookup(&UIP_IP_BUF->srcipaddr);
    if(nbr == NULL) {
    	/* The NCE does not exist. Try to create it in TENTATIVE state. */
			nbr = pgw_nbr_add(&UIP_IP_BUF->srcipaddr,
												(uip_lladdr_t *)&(pgw_opt_aro->eui64),
												0, PGW_TENTATIVE);
			if (nbr == NULL) {
				/* NC is full. We must respond a NA reporting the error */
       	pgw_registration_error(ARO_STATUS_RTR_NC_FULL);
			} else {
				/* Registration succeeded. Set NCE's ARO-pending flag = 1 */
				nbr->aro_pending = 1;
				/* 
				 * We set the timer now just in order to save the lifetime. However, we'll have 
				 * to restart it just before sending the NA in response. Note that no harm can
				 * result from this action, since after DAD completes, the NCE will be either 
				 * registered or deleted.
				 */
				stimer_set(&(nbr->reachable), uip_ntohs(pgw_opt_aro->lifetime) * 60);
				/* Perform DAD on behalf of 6LN */
				pgw_dad(nbr);
			}
    } else { /* nbr != NULL */
     	/* 
       * The NCE exists. We have to check which case we are in:
       * - Duplicate
       * - Registration
       * - Re-registration (if NCE exists in REGISTERED state)
       */
      if(!eui64_cmp(&(pgw_opt_aro->eui64), &(nbr->lladdr))) {
      	/* 
      	 * NCE exists with different EUI-64 (Duplicate). We must respond a NA 
      	 * reporting the error. In this case, we must not delete the NCE, since
      	 * it corresponds to another node. 
      	 */
      	pgw_registration_error(ARO_STATUS_DUPLICATE);
      } else { /* NCE exists with the same EUI-64 */
      	if(nbr->state == PGW_REGISTERED) {
       		/* 
       		 * Re-registration. Refresh Lifetime, set ARO-pending flag = 1 and Forward NS to 
       		 * IEEE 802.3 segment to perform NUD of the router.
       		 * We set the timer now just in order to save the lifetime. However, we'll have 
				 	 * to restart it just before responding the NA in response 
				 	 */
          stimer_set(&(nbr->reachable), uip_ntohs(pgw_opt_aro->lifetime) * 60); 
          nbr->aro_pending = 1;
        } else if (nbr->state == PGW_TENTATIVE){
        	/* 
           * The NCE has just been created but DAD has not finished yet. Thus, 
           * registration for this node is in progress and we can discard this packet. 
           */
          goto discard;
        } else {
        	/* 
        	 * Either the NCE exists in GARBAGE-COLLECTIBLE state or there has been an 
        	 * error somewhere. Discard 
        	 */
        	goto discard;
        }
      }
    }
		break;
	default:
		break;	
	}
	return;
discard:
	uip_len = 0;
forward:
	return;
}

static void
proxy_na_input()
{
/* 
 * We could process the packet to update the isRouter flag of the source, 
 * but that may be unnecessary extra processing. 
 */
	switch (incoming_if) {
	case IEEE_802_3:
	case LOCAL:
		if (uip_is_addr_linklocal_allnodes_mcast(&UIP_IP_BUF->destipaddr)) {
			/* 
			 * This NA message has been sent either for quick information 
			 * propagation or as response to a NS sent for DAD (which means DAD
			 * has failed).
			 */
			nbr = pgw_nbr_lookup(&(UIP_ND6_NA_BUF->tgtipaddr));
			if (nbr == NULL) {
				/* This message was sent for quick information propagation. Forward it */
				goto discard;
			} else if (nbr->state == PGW_TENTATIVE) {
				/* This message is the response to a NS sent for DAD */
				pgw_dad_failed(nbr); 
			}
		} else {
			/* 
			 * This message can only be originated as response to a NS which
			 * could have been sent for NUD or for registration (including an ARO).
			 * Check if the source IPv6 address is the IPv6 address of the RR. 
			 */
			nbr = pgw_nbr_lookup(&(UIP_IP_BUF->destipaddr));
			if (nbr == NULL) {
				goto discard;
			} else {
				if ((nbr->aro_pending) && 
					((nbr->state == PGW_TENTATIVE) || (nbr->state == PGW_REGISTERED)) && 
					(uip_ipaddr_cmp(&rr_ipaddr, (uip_ipaddr_t*)&UIP_IP_BUF->srcipaddr))){
					/* 
					 * This message is the final part of the registration process. We must
					 * thus clear the ARO-pending flag, make sure the state is REGISTERED,
					 * and forward the NA APPENDING THE APPROPIATE ARO option with status
					 * field = SUCCESS.
					 */
					/* Make sure state is registered */
	        nbr->state = PGW_REGISTERED;
			    nbr->aro_pending = 0;
			    /* include ARO option */
					pgw_append_icmp_opt(UIP_ND6_OPT_ARO, &nbr->lladdr, ARO_STATUS_SUCCESS,
															uip_htons(nbr->reachable.interval / 60));
					/* re-compute checksum */
					pgw_update_icmp_checksum();
					/* Reset the timer */
					stimer_restart(&(nbr->reachable));
				} else if (nbr->state == PGW_REGISTERED){
					goto forward;
				} else {
					goto discard;
				}
			}
		} 
		break;
	case IEEE_802_15_4:
		/* 
		 * As we have chosen to respond to NS messages on behalf of 6LoWPAN nodes,
		 * it is unlikely that we will receive NA messages from the IEEE 802.15.4 
		 * segment. However, if they arrive, forward the packet unchanged.
		 */
		goto forward;
	default:
		break;	
	}
	return;
discard:
	uip_len = 0;
forward:
	return;
}

static void
proxy_rs_input() 
{
	
	switch (incoming_if) {
	case IEEE_802_3:
	case LOCAL:
		/* 
		 * Here we choose not to forward RS messages to the IEEE 802.15.4 
		 * segment, since that is probably useless in most cases.
		 */
		goto discard;
		//break;
	case IEEE_802_15_4:
		if (uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
			/* The source address MUST NOT be the unspecified */
			goto discard;
		}
		
		pgw_opt_llao = NULL;
		pgw_opt_offset = UIP_ND6_RS_LEN;
  	while(uip_l3_icmp_hdr_len + pgw_opt_offset < uip_len) {
			if(UIP_PGW_OPT_HDR_BUF->len == 0) {
				goto discard;
    	}
    	if (UIP_PGW_OPT_HDR_BUF->type == UIP_ND6_OPT_SLLAO) {
    		pgw_opt_llao = &uip_buf[uip_l2_l3_icmp_hdr_len + pgw_opt_offset];
    	}
    	pgw_opt_offset += CURRENT_OPT_LENGTH;
  	}
		if (pgw_opt_llao == NULL) {
			/* An SLLAO option MUST be included (I-D.ietf-6lowpan-nd)*/
			goto discard;
		}
		/* 
		 * Set the ra-pending flag in NCE. If the NCE does not exist, create it
		 * (even if the source address is link-local, since I-D.ietf-6lowpan-nd 
		 * permits it). There is no problem if the NC is full since the is no 
		 * interest either in providing a prefix for a node that can not get 
		 * registered to the 6LP-GW. 
		 */
		nbr = pgw_nbr_lookup(&UIP_IP_BUF->srcipaddr);
    if(nbr == NULL) {
   		/* The NCE does not exist. Try to create it in TENTATIVE state. */
			nbr = (pgw_nbr_add(&UIP_IP_BUF->srcipaddr,
										(uip_lladdr_t *)&(pgw_opt_llao[UIP_ND6_OPT_DATA_OFFSET]),
										0, PGW_TENTATIVE));
    } 
		if (nbr != NULL) {
    	nbr->ra_pending = 1;
    	/* Set also global ra_pending flag */
    	ra_pending = 1;
		}
		break;
	default:
		break;	
	}
	return;
discard:
	uip_len = 0;
}

static void
proxy_ra_input()
{
	u8_t aux_len;
	
	switch (incoming_if) {
	case IEEE_802_3:
		/* First of all, retrieve RR's IPv6 address and MAC address! */
		eui64_copy(&rr_lladdr, &src_eui64);
		uip_ipaddr_copy(&rr_ipaddr, (uip_ipaddr_t *)&UIP_IP_BUF->srcipaddr);
		/* 
		 * Process RA options. We need to make sure that:
		 * - The packet contains a SLLAO option
		 * - The 'L' on-link flag in the PIO option is clear.
		 */
		pgw_opt_llao = NULL;
		pgw_opt_offset = UIP_ND6_RA_LEN;
  	while(uip_l3_icmp_hdr_len + pgw_opt_offset < uip_len) {
			if(UIP_PGW_OPT_HDR_BUF->len == 0) {
				goto discard;
    	}
    	if (UIP_PGW_OPT_HDR_BUF->type == UIP_ND6_OPT_SLLAO) {
    		pgw_opt_llao = &uip_buf[uip_l2_l3_icmp_hdr_len + pgw_opt_offset];
    	} else if (UIP_PGW_OPT_HDR_BUF->type == UIP_ND6_OPT_PREFIX_INFO) {
    		pgw_opt_prefix_info = (uip_nd6_opt_prefix_info *)&uip_buf[uip_l2_l3_icmp_hdr_len + pgw_opt_offset];
    	}
    	pgw_opt_offset += CURRENT_OPT_LENGTH;
  	}
		if (pgw_opt_prefix_info != NULL) {
			/* If there is a PIO option, make sure the 'L' on-link flag is clear */
			pgw_opt_prefix_info->flagsreserved1 &= ~UIP_ND6_RA_FLAG_ONLINK;
			/* Also use the network prefix to create/update a context entry. If at some
			 * point the network prefix announced in RA messages changes, the corresponding
			 * context will eventually expire with no hazards */
			context = pgw_context_lookup_by_prefix(&pgw_opt_prefix_info->prefix);
			if (context == NULL) {
				context = pgw_context_create(&pgw_opt_prefix_info->prefix, 
														pgw_opt_prefix_info->preflen);
				if (context != NULL) {										
					context_chaged = 1;
				}
			} else if (context->state == IN_USE_COMPRESS) {
				/* Reset timer */
				stimer_reset(&context->vlifetime);
			} else if (context->state == EXPIRED) {
				/* Set timer and return to IN_USE_COMPRESS state */
				context->state = IN_USE_COMPRESS;
				stimer_set(&context->vlifetime, PGW_CONTEXT_LIFETIME);
				context_chaged = 1;
			}
		}
		/* After handling contexts and RR's addresses check whether continueing 
		 * is needed. */
		if (!ra_pending && !context_chaged) {
			goto discard;
		}
		/* This RA will be forwarded */
		
		if (pgw_opt_llao == NULL) {
			/* If there is no SLLAO option, append it */
			pgw_append_icmp_opt(UIP_ND6_OPT_SLLAO, &src_eui64, 0, 0);
		}
		/* Always append a 6CO option per context in use */
		for(context = pgw_addr_context_table;
      context < pgw_addr_context_table + SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS; context++) {
      if (context->state != NOT_IN_USE) {
				pgw_append_icmp_opt(UIP_ND6_OPT_6CO, context, 0, 0);
      }
		}
		/* The most likely situation is that there is going to be at least one
		 * context, so always update checksum */
		pgw_update_icmp_checksum();
		
		
		if (context_chaged) {
			/* We are going to send out a multicast RA */
			if(ra_pending) {
				/* Since all 6LNs are going to receive a RA, we must clear the 
				 * RA-pending flag for every NCE*/
				for(nbr = pgw_6ln_cache; nbr < pgw_6ln_cache + MAX_6LOWPAN_NEIGHBORS; nbr++) {
					if(nbr->isused && nbr->ra_pending) {
						nbr->ra_pending = 0;
					}
				}	 
				ra_pending = 0;  
			}
			if (!uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
				/* Set destination IPv6 address*/
				uip_create_linklocal_allnodes_mcast(&UIP_IP_BUF->destipaddr);
				/* And destination MAC address */
				eui64_copy(&dst_eui64, &rimeaddr_null);
			}
			context_chaged = 0;
		} else { /* ra_pending is necessarily 1 */
			/* If the destination address is the all-nodes multicast address, we
			 * choose to send it only to the 6LNs whose NCEs are marked as 
			 * ra_pending. Since routers may choose to delay the router advertisement
			 * in order to respond to several solicitations, we have to send the RA 
			 * to all marked NCEs. */
			if (uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
				/* Save uip_len, as pgw_output sets it to 0 */
				aux_len = uip_len;
				for(nbr = pgw_6ln_cache; nbr < pgw_6ln_cache + MAX_6LOWPAN_NEIGHBORS; nbr++) {
					if(nbr->isused && nbr->ra_pending) {
						/* Retrieve uip_len (we might have cleared it if we've sent a packet) */
						uip_len = aux_len;
						/* Set destination IPv6 address*/
						uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &nbr->ipaddr);
						/* And destination MAC address */
						eui64_copy(&dst_eui64, &nbr->lladdr);
						/* Send packet out */
						pgw_output();
						/* Clear the RA-pending flag */
						nbr->ra_pending = 0;
					}
				}
				ra_pending = 0;
			} else {
				/* 
				 * The RA message is unicast. Clear NCE's ra_pending flag in case 
				 * the NCE exists. Otherwise discard the message.
				 */
				nbr = pgw_nbr_lookup(&UIP_IP_BUF->destipaddr);
	      if(nbr != NULL) {
					nbr->ra_pending = 0;
					/* As we only sent the RA to 1 node, we must recalculate the value 
					 * of the global ra_pending */
					ra_pending = 0; 
					for(nbr = pgw_6ln_cache; nbr < pgw_6ln_cache + MAX_6LOWPAN_NEIGHBORS; nbr++) {
						if(nbr->isused && nbr->ra_pending) {
							ra_pending = 1;
							break;
						}
					}
	      } else {
	      	goto discard;
	      }
			}
		}
		break;
	case IEEE_802_15_4:
	case LOCAL:
		/* 
		 * In order to mask the router nature of 6LRs, we choose also
		 * not to forward RA messages originated in the IEEE 802.15.4 segment.
		 * In addition, we do not expect the local host to generate RAs
		 */
		goto discard;
		//break;
	default:
		break;	
	}
	return;
discard:
	uip_len = 0;
}

static void
proxy_redirect_input()
{
	uip_len = 0;
	return;
}

static void
pgw_output(){
	if (uip_len > 0) {
		pgw_fwd_output(&src_eui64, &dst_eui64);
		uip_len = 0;
	}
}

/* This function neither includes any option nor calculates the ICMP checksum,
 * but it does update the uip_len value and ip header length value.
 */
void pgw_create_ns(uip_ipaddr_t* src, uip_ipaddr_t* dst, uip_ipaddr_t* tgt){
	
	uip_ipaddr_t aux;
	/* 
	 * Deep copy address to ensure that addresses are not overwriten 
	 * (in case src and/or dst point to each other).
	 */
	uip_ipaddr_copy(&aux, src);
	
	if (dst == NULL) {
		uip_create_solicited_node(tgt, &UIP_IP_BUF->destipaddr);
	} else {
		uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, dst);	
	}
	
	if (src == NULL) {
		uip_create_unspecified(&UIP_IP_BUF->srcipaddr);
	} else {
		uip_ipaddr_copy(&UIP_IP_BUF->srcipaddr, &aux);	
	}
	
	uip_ext_len = 0;
	UIP_IP_BUF->vtc = 0x60;
	UIP_IP_BUF->tcflow = 0;
	UIP_IP_BUF->flow = 0;
	UIP_IP_BUF->len[0] = 0;       /* length will not be more than 255 */
	UIP_IP_BUF->len[1] = UIP_ICMPH_LEN + UIP_ND6_NS_LEN;
	UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
	UIP_IP_BUF->ttl = UIP_ND6_HOP_LIMIT;

	UIP_ICMP_BUF->type = ICMP6_NS;
	UIP_ICMP_BUF->icode = 0;
	
	UIP_ND6_NS_BUF->reserved = 0;
	
	uip_ipaddr_copy((uip_ipaddr_t *)&UIP_ND6_NS_BUF->tgtipaddr, tgt);

	uip_len =
    	UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NS_LEN;
}

/* This function neither includes any option nor calculates the ICMP checksum,
 * but it does update the uip_len value and ip header length value.
 */
void
pgw_create_na(uip_ipaddr_t* src, uip_ipaddr_t* dst, uip_ipaddr_t* tgt, u8_t flags){
		
	uip_ipaddr_t aux;
	
	uip_ipaddr_copy(&aux, src);
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, dst);
	uip_ipaddr_copy(&UIP_IP_BUF->srcipaddr, &aux);
	
	uip_ext_len = 0;
	UIP_IP_BUF->vtc = 0x60;
	UIP_IP_BUF->tcflow = 0;
	UIP_IP_BUF->flow = 0;
	UIP_IP_BUF->len[0] = 0;       /* length will not be more than 255 */
	UIP_IP_BUF->len[1] = UIP_ICMPH_LEN + UIP_ND6_NA_LEN;
	UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
	UIP_IP_BUF->ttl = UIP_ND6_HOP_LIMIT;

	UIP_ICMP_BUF->type = ICMP6_NA;
	UIP_ICMP_BUF->icode = 0;

	UIP_ND6_NA_BUF->flagsreserved = flags;
	
	uip_ipaddr_copy((uip_ipaddr_t *)&UIP_ND6_NA_BUF->tgtipaddr, tgt);

	uip_len =
    	UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NA_LEN;
}

/* This function appends the ICMPv6 option specified in type. It does not 
 * calculate the checksum but it does update the uip_len value and ip header
 * length value 
 */
void
pgw_append_icmp_opt(u8_t type, void* data, u8_t status, u16_t lifetime)
{

	UIP_ICMP_OPTS_APPEND->type = type;
	/* Length depends on the specific type of option */
	switch (type) {
	case UIP_ND6_OPT_SLLAO:
	case UIP_ND6_OPT_TLLAO:
		UIP_ICMP_OPTS_APPEND->len = UIP_ND6_OPT_LLAO_LEN >> 3;
  	memcpy((u8_t*)(UIP_ICMP_OPTS_APPEND) + UIP_ND6_OPT_DATA_OFFSET, data, UIP_LLADDR_LEN);
  	/* padding required */
  	memset((u8_t*)(UIP_ICMP_OPTS_APPEND) + UIP_ND6_OPT_DATA_OFFSET + UIP_LLADDR_LEN, 0,
    				UIP_ND6_OPT_LLAO_LEN - 2 - UIP_LLADDR_LEN);
    UIP_IP_BUF->len[1] += UIP_ND6_OPT_LLAO_LEN;
    uip_len += UIP_ND6_OPT_LLAO_LEN;
		break;
	case UIP_ND6_OPT_ARO:
		UIP_ICMP_OPTS_APPEND->len = UIP_ND6_OPT_ARO_LEN >> 3;
		((uip_nd6_opt_aro*)UIP_ICMP_OPTS_APPEND)->status = status;
		/* The reserved field MUST be initialized to zero by the sender */		
		((uip_nd6_opt_aro*)UIP_ICMP_OPTS_APPEND)->reserved1 = (u8_t)0;
 		((uip_nd6_opt_aro*)UIP_ICMP_OPTS_APPEND)->reserved2 = (u16_t)0;
  	((uip_nd6_opt_aro*)UIP_ICMP_OPTS_APPEND)->lifetime = lifetime;
		memcpy(&(((uip_nd6_opt_aro*)UIP_ICMP_OPTS_APPEND)->eui64), data, UIP_LLADDR_LEN);
		/* No need for padding here */
		UIP_IP_BUF->len[1] += UIP_ND6_OPT_ARO_LEN;
		uip_len += UIP_ND6_OPT_ARO_LEN;
		break;
	case UIP_ND6_OPT_6CO:
		if (((pgw_addr_context_t*)data)->length > 64) {
			UIP_ICMP_OPTS_APPEND->len = 3; /* Units of 8 octets */
		} else {
			UIP_ICMP_OPTS_APPEND->len = 2; /* Units of 8 octets */
		}
		((uip_nd6_opt_6co*)UIP_ICMP_OPTS_APPEND)->preflen = ((pgw_addr_context_t*)data)->length;
		((uip_nd6_opt_6co*)UIP_ICMP_OPTS_APPEND)->res1_c_cid = ((pgw_addr_context_t*)data)->context_id & 0x0F; 
		if (((pgw_addr_context_t*)data)->state == IN_USE_COMPRESS) {
			((uip_nd6_opt_6co*)UIP_ICMP_OPTS_APPEND)->res1_c_cid |= UIP_ND6_RA_FLAG_COMPRESSION; 
		}
		((uip_nd6_opt_6co*)UIP_ICMP_OPTS_APPEND)->reserved2 = (u16_t)0;
		((uip_nd6_opt_6co*)UIP_ICMP_OPTS_APPEND)->lifetime = 
				uip_htons(((pgw_addr_context_t*)data)->vlifetime.interval / 60); /*units of 60s*/
		memcpy(&(((uip_nd6_opt_6co*)UIP_ICMP_OPTS_APPEND)->prefix), 
						&((pgw_addr_context_t*)data)->prefix,
						((pgw_addr_context_t*)data)->length);
		UIP_IP_BUF->len[1] += UIP_ICMP_OPTS_APPEND->len << 3;
    uip_len += UIP_ICMP_OPTS_APPEND->len << 3;
		break;	
	}
}

/* Updates the ICMPv6 checksum */
void
pgw_update_icmp_checksum(){
	UIP_ICMP_BUF->icmpchksum = 0;
	UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();
}

/* This function expects a NS with ARO to be in uip_buf and generates
 * the NA with ARO in response depending on the value of status. */
static void
pgw_registration_error(u8_t status) {
	pgw_create_na(&rr_ipaddr, &UIP_IP_BUF->srcipaddr, &UIP_ND6_NS_BUF->tgtipaddr, 
					UIP_ND6_NA_FLAG_ROUTER | 
					UIP_ND6_NA_FLAG_SOLICITED | 
					UIP_ND6_NA_FLAG_OVERRIDE);
	
	/* include TLLAO option */
	pgw_append_icmp_opt(UIP_ND6_OPT_TLLAO, 
						(eui64_t*)&(pgw_opt_llao[UIP_ND6_OPT_DATA_OFFSET]), 0, 0);
	/* include ARO option */
	pgw_append_icmp_opt(UIP_ND6_OPT_ARO, (eui64_t*)&(pgw_opt_aro->eui64), 
						status, pgw_opt_aro->lifetime);
	/* Compute checksum */
	pgw_update_icmp_checksum();
	/* Set outgoing interface and src and dst MAC addresses */
	outgoing_if = IEEE_802_15_4;
	eui64_copy(&aux_eui64, &src_eui64);
	eui64_copy(&src_eui64, &dst_eui64);
	eui64_copy(&dst_eui64, &aux_eui64);
}

/*
 * This function will be called only when THIS NODE has a packet to send;
 * the packet to be sent is in uip_buf, the destination MAC address is in 
 * localdest and the source MAC address is this node's MAC address, which can 
 * be found in either rimeaddr_node_addr or in uip_lladdr.
 */
void 
local_node_output(uip_lladdr_t *localdest) {	
	/* Set incoming interface = LOCAL so pgw_input() can handle this packet
	 * properly */
	incoming_if = LOCAL;
	
	if (localdest == NULL) {
		eui64_copy(&dst_eui64, &rimeaddr_null);
	} else {
		eui64_copy(&dst_eui64, (eui64_t*)localdest);
	}
	
	eui64_copy(&src_eui64, &rimeaddr_node_addr);
	//TODO: replace by pgw_input()?
	pgw_input();
//	pgw_packet_input();
//	pgw_output();
}

/*---------------------------------------------------------------------------*/

static void
pgw_eventhandler(process_event_t ev, process_data_t data)
{
	if (ev == PROCESS_EVENT_TIMER) { 
		if(data == &pgw_timer_periodic && etimer_expired(&pgw_timer_periodic)) {
			pgw_periodic();
			pgw_output();
			etimer_reset(&pgw_timer_periodic);	
		}
	}
}

void
pgw_input(void)
{
	pgw_packet_input();
	pgw_output();
}

PROCESS_THREAD(pgw_process, ev, data)
{
	PROCESS_BEGIN();
  
	while(1) {
    	PROCESS_YIELD();
    	pgw_eventhandler(ev, data);
	}
	
	PROCESS_END();
}

const struct pgw_driver proxy_gateway_driver = {
  "proxy_gateway_driver",
  pgw_init,
  pgw_input,
  local_node_output
};

