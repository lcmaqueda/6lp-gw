/**
 * \file		pgw_fwd.h
 *
 * \brief		Forwarding/bridging-related definitions for the 6LoWPAN-ND proxy-gateway 
 *
 * \author		Luis Maqueda <luis@sen.se>
 */


#ifndef PGW_FWD_H_
#define PGW_FWD_H_

#include "net/p-gw/pgw.h"
#include "net/p-gw/pgw_nd.h"
#include "contiki.h"
#include "contiki-net.h"


/* Number of entries in the bridge */
#define MAX_BRIDGE_ENTRIES	30

/* 
 * Interface types 
 */
typedef enum {
	UNDEFINED = 0,
	IEEE_802_3,
	IEEE_802_15_4,
	LOCAL
} interface_t;

/* 
 * An entry in the bridge cache 
 */
typedef struct {
	interface_t interface;
	eui64_t addr;
} bridge_entry_t;

typedef struct {
	bridge_entry_t table[MAX_BRIDGE_ENTRIES];
	u8_t elems;
} bridge_table_t;

/** \brief Incoming and outgoing interfaces */
extern interface_t incoming_if, outgoing_if;
/** \brief Source and destination MAC addresses */
extern eui64_t src_eui64, dst_eui64;


/* Packet forwarding and address translation functions */
void pgw_fwd_init(void);
void pgw_fwd_input(void);
void pgw_fwd_output(eui64_t* src, eui64_t* dst);

#endif /*PGW_FWD_H_*/
