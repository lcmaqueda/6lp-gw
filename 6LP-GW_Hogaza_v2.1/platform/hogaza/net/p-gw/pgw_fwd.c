/**
 * \file		pgw_fwd.c
 *
 * \brief		Forwarding/bridging-related functions for the 6LoWPAN-ND proxy-gateway 
 *
 * \author		Luis Maqueda <luis@sen.se>
 */
#include "net/p-gw/pgw_fwd.h"
#include "net/uip-nd6.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define ETH_BUF 	((struct uip_eth_hdr *)&uip_buf[0])
#define UIP_ICMP_BUF     ((struct uip_icmp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_PGW_OPT_HDR_BUF  ((uip_nd6_opt_hdr *)&uip_buf[uip_l2_l3_icmp_hdr_len + pgw_opt_offset])
#define CURRENT_OPT_LENGTH		(UIP_PGW_OPT_HDR_BUF->len << 3)

/*
 *  Global (extern) variables 
 */
interface_t incoming_if = UNDEFINED;
interface_t outgoing_if = UNDEFINED;
eui64_t src_eui64, dst_eui64;									/** \brief Source and destination MAC addresses */

/* 
 * Static Variables 
 */
/** \brief Bridge table */
static bridge_table_t brigde_table;
/** \brief Bridge table entry*/
static bridge_entry_t *lookup_result;
/** Offset from the end of the icmpv6 header to the option in uip_buf*/					
static u8_t pgw_opt_offset;
/**  Pointer to llao option in uip_buf */
static u8_t *pgw_opt_llao;
/* 
 * Local function prototypes 
 */
static void bridge_input(void);
static void bridge_addr_add(eui64_t *addr, interface_t interface);
static bridge_entry_t* bridge_addr_lookup(eui64_t *addr); 
static int is_multicast_lladdr(eui64_t* addr);
static u8_t translate_icmp_lladdr(interface_t target);
static u8_t network_layer_filter(void);
static void get_lladdr(eui64_t* src, eui64_t* dst);
//static void slide(u8_t* data, int16_t len, int16_t slide);
static void radio_if_forward(eui64_t* src, eui64_t* dst);
static void eth_if_forward(eui64_t* src, eui64_t* dst);
static void create_ethernet_lladdr(eui64_t * ethernet, eui64_t * lowpan);
static void create_6lowpan_lladdr(eui64_t * ethernet, eui64_t * lowpan);

void
pgw_fwd_init() 
{
	memset(brigde_table, 0, sizeof(brigde_table));
	/* Optimization: Add local host to bridge cache */
	bridge_addr_add(&rimeaddr_node_addr, LOCAL);
}

void
pgw_fwd_input()
{
	
	/* get src and dst MAC addresses */
	if (incoming_if != LOCAL) {
		get_lladdr(&src_eui64, &dst_eui64);
	}
	bridge_input();
	
	if (network_layer_filter()) {
		uip_len = 0;
	}
}

static void
bridge_input() 
{
	
	lookup_result = bridge_addr_lookup(&src_eui64); 
	if (lookup_result == NULL) {
		/* 
		 * If the src MAC address is not in the bridge cache, add the
		 * pair (MAC addr., interface) to the cache.
		 */
		bridge_addr_add(&src_eui64, incoming_if);
	}
	
	lookup_result = bridge_addr_lookup(&dst_eui64);
	if ((is_multicast_lladdr(&dst_eui64)) || (lookup_result == NULL)) {
		/* 
	 	 * If it is multicast packet or a unicast packet whose dst. MAC addr. 
	 	 * is not in the cache, forward it to every interface but the upstream 
	 	 * interface (including This Node's interface).
	 	 */			
	 	outgoing_if = UNDEFINED;
	} else {
		outgoing_if = lookup_result->interface;
	}
}

void 
pgw_fwd_output(eui64_t* src, eui64_t* dst) 
{
//TODO: rearrange possible?
		if (outgoing_if == UNDEFINED) {
			/* 
			 * It could be a multicast packet. Forward it to every interface but the 
			 * incoming interface (including LOCAL Interface)
			 */
			if (incoming_if == IEEE_802_3) {
				/* Forward it to the IEEE_802_15_4 interface */
				outgoing_if = IEEE_802_15_4;
				translate_icmp_lladdr(IEEE_802_15_4);
				radio_if_forward(src, dst);
				/* And also to the LOCAL interface */
				translate_icmp_lladdr(IEEE_802_15_4); //Already done in radio_if_forward()
				tcpip_input();	
			} else if (incoming_if == IEEE_802_15_4) {
				/* Forward it to the Ethernet interface first*/
				outgoing_if = IEEE_802_3;
				translate_icmp_lladdr(IEEE_802_3);
				eth_if_forward(src, dst);	
				/* And also to the LOCAL interface */
				translate_icmp_lladdr(IEEE_802_15_4); // tcpip stack works with 8-byte addresses
				tcpip_input();
			} else if (incoming_if == LOCAL) {
				/* Forward it to the IEEE_802_15_4 interface */
				outgoing_if = IEEE_802_15_4;
				radio_if_forward(src, dst);
				outgoing_if = IEEE_802_3;
				/* And also to the IEEE_802_3 interface */
				translate_icmp_lladdr(IEEE_802_3);
				eth_if_forward(src, dst);
				uip_len = 0;
			} else {
				/* This should never happen */
				goto discard;
			}
	} else {
		if (outgoing_if == LOCAL) {
			if (incoming_if == IEEE_802_3) {
				/* A link layer addresses translation is needed before passing to the stack */
				translate_icmp_lladdr(IEEE_802_15_4);
			}
			tcpip_input();
		} else if (outgoing_if == IEEE_802_15_4) {
			if (incoming_if == IEEE_802_3) {
				/* A link layer addresses conversion is needed before forwarding */
				translate_icmp_lladdr(IEEE_802_15_4);
			} 
			radio_if_forward(src, dst);
		} else if (outgoing_if == IEEE_802_3) {
			translate_icmp_lladdr(IEEE_802_3);
			eth_if_forward(src, dst);
		} else {
			goto discard;
		}
	}
	return;
discard:
	uip_len = 0;
	return;
}

/*
 * \brief		Returns the link-layer addresses. Note that the addresses are always
 *	 				8-byte long, so if the link-layer is Ethernet, it is first converted
 * 					to the corresponding 8-byte address.
 */
static void 
get_lladdr(eui64_t* src, eui64_t* dst) 
{
	
	if(incoming_if == IEEE_802_15_4) {
		/* The packet came from the IEEE_802_15_4 interface */
		eui64_copy(src, (eui64_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
		/* If the dst. addr. is multicast, frame802154::parse put the rimeaddr_null address here */
		eui64_copy(dst, (eui64_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
	} else if (incoming_if == IEEE_802_3) {
		/* 
		 * The packet came from the Ethernet interface.
		 * Create 8-byte src MAC address from ethernet src address.
		 */
		create_6lowpan_lladdr((eui64_t *)&(((struct uip_eth_hdr *) &uip_buf[0])->src), src);
		if(((struct uip_eth_hdr *) &uip_buf[0])->dest.addr[0] == 0x33 &&
			((struct uip_eth_hdr *) &uip_buf[0])->dest.addr[1] == 0x33) {
			/*
			 * It is an Ethernet multicast address. Assign the rimeaddr_null address.
			 */
			eui64_copy(dst, &rimeaddr_null);
		} else {
			create_6lowpan_lladdr((eui64_t *)&(((struct uip_eth_hdr *) &uip_buf[0])->dest), dst);
		}
	} else {
		return;
	}	
}

/**
 * \brief 			Create a 802.15.4 long address from a 802.3 address. 
 * 					The address is assigned in such a way that it is 
 * 					possible that ethernet and lowpan point to the same
 * 					address.
 * 				
 * \param ethernet	Pointer to ethernet address
 * \param lowpan    Pointer to 802.15.4 address
 */
static void 
create_6lowpan_lladdr(eui64_t * ethernet, eui64_t * lowpan)
{
  lowpan->u8[0] = ethernet->u8[0];
  lowpan->u8[1] = ethernet->u8[1];
  lowpan->u8[2] = ethernet->u8[2];
  lowpan->u8[7] = ethernet->u8[5];
  lowpan->u8[5] = ethernet->u8[3];
  lowpan->u8[6] = ethernet->u8[4];
  lowpan->u8[3] = 0xff;
  lowpan->u8[4] = 0xfe;
}

/**
 * \brief Returns True if addr is a multicast address. Note that in the 
 * processing logic, multicast addresses are those having all-zeroes.
 */
int is_multicast_lladdr(eui64_t* addr) {
	return eui64_cmp(addr, &rimeaddr_null);
}

/**
 * \brief Translate the link-layer (L2) addresses in an ICMP packet.
 *        This will just be NA/NS/RA/RS packets currently.
 * 		  It Also removes undesired options to optimize transmissions
 * \param target The target we want to end up with - either IEEE_802_3
 *        for ethernet, or IEEE_802_15_4 for 802.15.4
 * \return       Returns how successful the translation was
 * \retval 0     Addresses, if present, were translated.
 * \retval -1    ICMP message was unknown type, nothing done.
 * \retval -2    ICMP Length does not make sense?
 * \retval -3    Unknown 'target' type
 */
 
static u8_t
translate_icmp_lladdr(interface_t target)
{
	if (UIP_IP_BUF->proto != UIP_PROTO_ICMP6) {
		return 0;
	}
	
	switch(UIP_ICMP_BUF->type) {
  case ICMP6_NS:
/*		NS and NA have same length
    	pgw_opt_offset = UIP_ND6_NS_LEN;
    	break; */
  case ICMP6_NA:
  	pgw_opt_offset = UIP_ND6_NA_LEN;
    break;
  case ICMP6_RS:
   	pgw_opt_offset = UIP_ND6_RS_LEN;
   	break;
  case ICMP6_RA:
   	pgw_opt_offset = UIP_ND6_RA_LEN;
   	break;
	default:
   	return 0;
	}
	
	while(uip_l3_icmp_hdr_len + pgw_opt_offset < uip_len) {
		if(UIP_PGW_OPT_HDR_BUF->len == 0) {
			return 0;
    }
   	if ((UIP_PGW_OPT_HDR_BUF->type == UIP_ND6_OPT_SLLAO) ||
   			(UIP_PGW_OPT_HDR_BUF->type == UIP_ND6_OPT_TLLAO)) {
   		pgw_opt_llao = (u8_t*)UIP_PGW_OPT_HDR_BUF;
     	/* Shrink/grow buffer as needed */
     	if (target == IEEE_802_15_4) {
       	/* 
       	 * Current link-layer address is 6 bytes long. As ICMPv6 options
       	 * length is expressed in units of 8 octets, new LLAO option will 
       	 * be 8 bytes longer.
       	 */
       	memmove(pgw_opt_llao + CURRENT_OPT_LENGTH + 8, 
       				pgw_opt_llao + CURRENT_OPT_LENGTH, 
       				uip_len - uip_l3_icmp_hdr_len - pgw_opt_offset - CURRENT_OPT_LENGTH);
//       	slide(pgw_opt_llao + CURRENT_OPT_LENGTH, 
//       				uip_len - uip_l3_icmp_hdr_len - pgw_opt_offset - CURRENT_OPT_LENGTH, 
//       				8);
       	/* Adjust the IP header length, as well as uIP length */		
       	uip_len += 8;
       	UIP_IP_BUF->len[1] = (u8_t)(uip_len - UIP_IPH_LEN);
				UIP_IP_BUF->len[0] = (((u8_t)(uip_len - UIP_IPH_LEN)) >> 8);
				/* Translate addresses */
       	create_6lowpan_lladdr((eui64_t *)&(pgw_opt_llao[UIP_ND6_OPT_DATA_OFFSET]), 
       														(eui64_t *)&(pgw_opt_llao[UIP_ND6_OPT_DATA_OFFSET]));
       	/* Fill the rest of the option with zeroes */
				memset(&(pgw_opt_llao[UIP_ND6_OPT_DATA_OFFSET]) + 8, 0, 6);
				/* Adjust the length of the option */
       	UIP_PGW_OPT_HDR_BUF->len = 2;
       	//We broke ICMP checksum, be sure to fix that
     		pgw_update_icmp_checksum();
			} else if (target == IEEE_802_3) {
				/* create eth address from 802.15.4 address before destroying it */
      	create_ethernet_lladdr((eui64_t*)&(pgw_opt_llao[UIP_ND6_OPT_DATA_OFFSET]), 
      															(eui64_t *)&(pgw_opt_llao[UIP_ND6_OPT_DATA_OFFSET]));
        /* 
         * Current link-layer address is 8 bytes long. As ICMPv6 options
         * length is expressed in units of 8 octets, new LLAO option will 
         * be 8 bytes shorter.
         */
         memmove(pgw_opt_llao + CURRENT_OPT_LENGTH - 8, 
         					pgw_opt_llao + CURRENT_OPT_LENGTH,
         					uip_len - uip_l3_icmp_hdr_len - pgw_opt_offset - CURRENT_OPT_LENGTH);
//        slide(pgw_opt_llao + CURRENT_OPT_LENGTH, 
//        			uip_len - uip_l3_icmp_hdr_len - pgw_opt_offset - CURRENT_OPT_LENGTH, 
//        			-8);
				/* Adjust the IP header length, as well as uip_len value */
        uip_len -= 8;
        UIP_IP_BUF->len[1] = (u8_t)(uip_len - UIP_IPH_LEN);
				UIP_IP_BUF->len[0] = (((u8_t)(uip_len - UIP_IPH_LEN)) >> 8);
	      /* Translate addresses */
      	/* Adjust the length of the option */
        UIP_PGW_OPT_HDR_BUF->len = 1;
        /* We broke ICMP checksum, be sure to fix that */
      	pgw_update_icmp_checksum();
			} else {
				return 0; //Uh-oh!
      }
#if CONF_OPT_FILTERING
    } else if (((target == IEEE_802_15_4) &&
    		(UIP_ICMP_BUF->type == ICMP6_RA) &&
    		(UIP_PGW_OPT_HDR_BUF->type != UIP_ND6_OPT_PREFIX_INFO) &&
//	    	(UIP_PGW_OPT_HDR_BUF->type != UIP_ND6_OPT_MTU) &&
	    	(UIP_PGW_OPT_HDR_BUF->type != UIP_ND6_OPT_6CO))) {
//	    			||
//	    			((target == IEEE_802_3) &&
//    				(UIP_PGW_OPT_HDR_BUF->type == UIP_ND6_OPT_6CO) &&
//	    			(UIP_PGW_OPT_HDR_BUF->type == UIP_ND6_OPT_ABRO) &&
//	    			(UIP_PGW_OPT_HDR_BUF->type == UIP_ND6_OPT_ARO))) {
	    /* Filter out options we don't care. Remove and slide */
			/* Update uip_len with its new value before we lose the length 
			 * of the option */
			uip_len -= CURRENT_OPT_LENGTH;
			/* Now, overwrite the option with the data that was immediately after it.
			 * Regarding the length, take into account that uip_buf has already been 
			 * updated! */
	    memmove((u8_t*)UIP_PGW_OPT_HDR_BUF,
	    				(u8_t*)UIP_PGW_OPT_HDR_BUF + CURRENT_OPT_LENGTH, 
    					uip_len - uip_l3_icmp_hdr_len - pgw_opt_offset);
//    	slide((u8_t*)UIP_PGW_OPT_HDR_BUF + CURRENT_OPT_LENGTH, 
//    				uip_len - uip_l2_l3_icmp_hdr_len - pgw_opt_offset - CURRENT_OPT_LENGTH,
//    				-CURRENT_OPT_LENGTH);
			
    	/* Adjust the IP header length */
      UIP_IP_BUF->len[1] = (u8_t)(uip_len - UIP_IPH_LEN);
			UIP_IP_BUF->len[0] = (u8_t)((uip_len - UIP_IPH_LEN) >> 8);

			/* We broke ICMP checksum, be sure to fix that */
      pgw_update_icmp_checksum();
		}
#else
		}
#endif /* CONF_OPT_FILTERING */
    pgw_opt_offset += CURRENT_OPT_LENGTH;
	}
	return 1;
}

/**
 * \brief 			Create a 802.3 address from a 802.15.4 long address. It is possible
 * 					that ethernet and lowpan point to the same address. 
 * \param ethernet  Pointer to ethernet address
 * \param lowpan    Pointer to 802.15.4 address
 */
static void 
create_ethernet_lladdr(eui64_t * ethernet, eui64_t * lowpan)
{
    ethernet->u8[0] = lowpan->u8[0];
    ethernet->u8[1] = lowpan->u8[1];
    ethernet->u8[2] = lowpan->u8[2];
    ethernet->u8[3] = lowpan->u8[5];
    ethernet->u8[4] = lowpan->u8[6];
    ethernet->u8[5] = lowpan->u8[7];
}

/**
 * \brief        Increase or decrease the size of the buffer, sliding
 * 				 up or down its contents.
 * \param data   Pointer from which the sliding occurs
 * \param length Length from data to the end of the buffer
 * \param slide  How many bytes to slide the buffer up in memory (if +) or
 *               down in memory (if -)
 */
//static void 
//slide(u8_t* data, int16_t len, int16_t slide) {
//	len--;
//	while (len >= 0) {
//		if (slide > 0) {
//			*(data + len + slide) = *(data + len);
//		} else {
//			*(data + slide) = *(data);
//			data++;
//		}
//		len--;
//	}
//}

static void 
radio_if_forward(eui64_t* src, eui64_t* dst)
{
	rimeaddr_t r_addr;
	uip_lladdr_t ll_addr;
	
	/*
	 * In order to use the sicslowpan output function, we need to set the
	 * global variable rimeaddr_node_addr with the src address of the 
	 * outgoing packet. This is the little hack that allows us to make use
	 * of the Contiki's sicslowpan compression method without needing to 
	 * change its code
	 */
	 eui64_copy(&r_addr, &rimeaddr_node_addr);
	 rimeaddr_set_node_addr(src);
	 
	/* In order to perform HC1 compression, the lowest 8 bytes of the IPv6 source 
	 * address are compared against the link layer address stored in the global
	 * variable uip_lladdr. Thus, a similar hack is needed again
	 */
	 eui64_copy((rimeaddr_t*)&ll_addr, (rimeaddr_t*)&uip_lladdr);
	 eui64_copy((rimeaddr_t*)&uip_lladdr, src);
	 
	/* if sicslowpan_init() was called and tcpip_set_outputfunc() was not
	 * called afterwards, tcpip_output() will evetually call the 6lowpan 
	 * output function, which will carry out the 6lowpan compression.
	 */
	if (is_multicast_lladdr(dst)) {
		/*
		 * tcpip_output() receives the destination MAC address as argument. If the
		 * destination address is the multicast address, it must receive NULL.
		 */
		NETSTACK_6LOWPAN.output(NULL);
	} else { 
		NETSTACK_6LOWPAN.output((uip_lladdr_t*)dst);
	}
	/*
	 * After sending out the packet, restore the original node address in 
	 * rimeaddr_node_addr and in uip_lladdr.
	 */
	rimeaddr_set_node_addr(&r_addr);
	eui64_copy((rimeaddr_t*)&uip_lladdr, (rimeaddr_t*)&ll_addr); 
}

static void 
eth_if_forward(eui64_t* src, eui64_t* dst)
{
	/*
	 * Copy the src and dst MAC addresses into uip_buf 
	 */
	create_ethernet_lladdr((eui64_t *)&ETH_BUF->src, (eui64_t *)src);
	if (is_multicast_lladdr(dst)) {
		/*
		 * Create Ethernet multicast address
		 */
		ETH_BUF->dest.addr[0] = 0x33;
	  ETH_BUF->dest.addr[1] = 0x33;
	  ETH_BUF->dest.addr[2] = UIP_IP_BUF->destipaddr.u8[12];
	  ETH_BUF->dest.addr[3] = UIP_IP_BUF->destipaddr.u8[13];
	  ETH_BUF->dest.addr[4] = UIP_IP_BUF->destipaddr.u8[14];
	  ETH_BUF->dest.addr[5] = UIP_IP_BUF->destipaddr.u8[15];
	} else {
		/*
		 * Create Ethernet unicast address
		 */
		create_ethernet_lladdr((eui64_t *)&ETH_BUF->dest, (eui64_t *)dst);
	}
	/*
	 * The Ethernet type/length value that matches IPv6 is 0x86dd.
	 */
	ETH_BUF->type = UIP_HTONS(0x86dd);
	
	NETSTACK_MAC_ETH.send();
}

/*
 * Returns a value other than 0 if the packet in uip_buf can be filtered out. 
 */
static void 
bridge_addr_add(eui64_t *addr, interface_t interface) 
{
	
	int index;
	
	if (brigde_table.elems != MAX_BRIDGE_ENTRIES) {
		index = brigde_table.elems;
		brigde_table.elems++;
	} else {
		/* pick victim randomly */
		index = random_rand()%MAX_BRIDGE_ENTRIES;
	}
	brigde_table.table[index].interface = interface;
	eui64_copy(&brigde_table.table[index].addr, addr);
}

static bridge_entry_t* 
bridge_addr_lookup(eui64_t *addr) 
{
	int i;
	
	for (i = 0; i < brigde_table.elems; i++) {
		if (eui64_cmp(addr, &(brigde_table.table[i].addr))) {
			return &(brigde_table.table[i]);
		}
	}
	return NULL;
}

static u8_t
network_layer_filter() 
{
/* Accept only ICMPv6 and UDP (CoAP)*/
	if ((UIP_IP_BUF->proto != UIP_PROTO_UDP) &&
			(UIP_IP_BUF->proto != UIP_PROTO_ICMP6)) {
		return 1;
	}

	return 0;
}
