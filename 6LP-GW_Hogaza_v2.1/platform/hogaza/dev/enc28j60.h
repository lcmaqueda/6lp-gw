#ifndef __ENC28J60_H__
#define __ENC28J60_H__

#include "contiki.h"


/*! \file enc28j60.h \brief Microchip ENC28J60 Ethernet Interface Driver. */
/**
******************************************************************************
 File Name	: 'enc28j60.h'
 Title		: Microchip ENC28J60 Ethernet Interface Driver
 Author		: Andreas Reinhardt (c) 2007, original source by Pascal Stang (c)2005
 Modified by	: Luis Maqueda
 Created		: 11/26/2007
 Revised		: 05/07/2010
 Version		: 0.1
 Target MCU	: TI MSP430 series
 Editor Tabs	: 4

/	\ingroup network
/	\defgroup enc28j60 Microchip ENC28J60 Ethernet Interface Driver (enc28j60.c)
/	\code #include "net/enc28j60.h" \endcode
/	\par Overview
/		This driver provides initialization and transmit/receive
/	functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
/ This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
/ chip, using an SPI interface to the host processor.
/
/ Revision: Adapted to work in msp430x54xx family (Hogaza Board)
/

*****************************************************************************
@{
*/
// ENC28J60 SPI port
// MOSI and MISO are in port 3, pins 1 and 2
#define ENC_SPI_PORT		P3OUT
#define ENC_SPI_PIN			P3IN
#define ENC_SPI_DIR			P3DIR
#define ENC_SPI_SEL			P3SEL
#define ENC_MOSI			1
#define ENC_MISO			2
// ENC28J60 control port
// CS and SCK are in port 3, pins 0 and 3
#define ENC_CS_PORT			P3OUT
#define ENC_CS_DIR			P3DIR
#define ENC_SCK_PORT		P3OUT
#define ENC_SCK_DIR			P3DIR
#define ENC_SCK_SEL			P3SEL
#define ENC_CS				0
#define ENC_SCK				3
// ENC28J60 reset port
// reset pin is in port 3, pin 3 and is low active
#define ENC_RESET_PORT		P1OUT
#define ENC_RESET			3

// ENC28J60 Control Registers
// Control register definitions are a combination of address,
// bank number, and Ethernet/MAC/PHY indicator bits.
// - Register address	(bits 0-4)
// - Bank number		(bits 5-6)
// - MAC/PHY indicator	(bit 7)
#define ADDR_MASK	0x1F
#define BANK_MASK	0x60
#define SPRD_MASK	0x80
// All-bank registers
#define EIE			0x1B
#define EIR			0x1C
#define ESTAT		0x1D
#define ECON2		0x1E
#define ECON1		0x1F
// Bank 0 registers
#define ERDPTL		(0x00|0x00)
#define ERDPTH		(0x01|0x00)
#define EWRPTL		(0x02|0x00)
#define EWRPTH		(0x03|0x00)
#define ETXSTL		(0x04|0x00)
#define ETXSTH		(0x05|0x00)
#define ETXNDL		(0x06|0x00)
#define ETXNDH		(0x07|0x00)
#define ERXSTL		(0x08|0x00)
#define ERXSTH		(0x09|0x00)
#define ERXNDL		(0x0A|0x00)
#define ERXNDH		(0x0B|0x00)
#define ERXRDPTL	(0x0C|0x00)
#define ERXRDPTH	(0x0D|0x00)
#define ERXWRPTL	(0x0E|0x00)
#define ERXWRPTH	(0x0F|0x00)
#define EDMASTL		(0x10|0x00)
#define EDMASTH		(0x11|0x00)
#define EDMANDL		(0x12|0x00)
#define EDMANDH		(0x13|0x00)
#define EDMADSTL	(0x14|0x00)
#define EDMADSTH	(0x15|0x00)
#define EDMACSL		(0x16|0x00)
#define EDMACSH		(0x17|0x00)
// Bank 1 registers
#define EHT0		(0x00|0x20)
#define EHT1		(0x01|0x20)
#define EHT2		(0x02|0x20)
#define EHT3		(0x03|0x20)
#define EHT4		(0x04|0x20)
#define EHT5		(0x05|0x20)
#define EHT6		(0x06|0x20)
#define EHT7		(0x07|0x20)
#define EPMM0		(0x08|0x20)
#define EPMM1		(0x09|0x20)
#define EPMM2		(0x0A|0x20)
#define EPMM3		(0x0B|0x20)
#define EPMM4		(0x0C|0x20)
#define EPMM5		(0x0D|0x20)
#define EPMM6		(0x0E|0x20)
#define EPMM7		(0x0F|0x20)
#define EPMCSL		(0x10|0x20)
#define EPMCSH		(0x11|0x20)
#define EPMOL		(0x14|0x20)
#define EPMOH		(0x15|0x20)
#define ERXFCON		(0x18|0x20)
#define EPKTCNT		(0x19|0x20)
// Bank 2 registers
#define MACON1		(0x00|0x40|0x80)
#define MACON3		(0x02|0x40|0x80)
#define MACON4		(0x03|0x40|0x80)
#define MABBIPG		(0x04|0x40|0x80)
#define MAIPGL		(0x06|0x40|0x80)
#define MAIPGH		(0x07|0x40|0x80)
#define MACLCON1	(0x08|0x40|0x80)
#define MACLCON2	(0x09|0x40|0x80)
#define MAMXFLL		(0x0A|0x40|0x80)
#define MAMXFLH		(0x0B|0x40|0x80)
#define MICMD		(0x12|0x40|0x80)
#define MIREGADR	(0x14|0x40|0x80)
#define MIWRL		(0x16|0x40|0x80)
#define MIWRH		(0x17|0x40|0x80)
#define MIRDL		(0x18|0x40|0x80)
#define MIRDH		(0x19|0x40|0x80)
// Bank 3 registers
#define MAADR5		(0x00|0x60|0x80)
#define MAADR6		(0x01|0x60|0x80)
#define MAADR3		(0x02|0x60|0x80)
#define MAADR4		(0x03|0x60|0x80)
#define MAADR1		(0x04|0x60|0x80)
#define MAADR2		(0x05|0x60|0x80)
#define EBSTSD		(0x06|0x60)
#define EBSTCON		(0x07|0x60)
#define EBSTCSL		(0x08|0x60)
#define EBSTCSH		(0x09|0x60)
#define MISTAT		(0x0A|0x60|0x80)
#define EREVID		(0x12|0x60)
#define ECOCON		(0x15|0x60)
#define EFLOCON		(0x17|0x60)
#define EPAUSL		(0x18|0x60)
#define EPAUSH		(0x19|0x60)
// PHY registers
#define PHCON1		0x00
#define PHSTAT1		0x01
#define PHID1		0x02
#define PHID2		0x03
#define PHCON2		0x10
#define PHSTAT2		0x11
#define PHIE		0x12
#define PHIR		0x13
#define PHLCON		0x14

// ENC28J60 EIE Register Bit Definitions
#define EIE_INTIE		0x80
#define EIE_PKTIE		0x40
#define EIE_DMAIE		0x20
#define EIE_LINKIE		0x10
#define EIE_TXIE		0x08
#define EIE_TXERIE		0x02
#define EIE_RXERIE		0x01
// ENC28J60 EIR Register Bit Definitions
#define EIR_PKTIF		0x40
#define EIR_DMAIF		0x20
#define EIR_LINKIF		0x10
#define EIR_TXIF		0x08
#define EIR_TXERIF		0x02
#define EIR_RXERIF		0x01
// ENC28J60 ESTAT Register Bit Definitions
#define ESTAT_INT		0x80
#define ESTAT_BUFER		0x40
#define ESTAT_LATECOL	0x10
#define ESTAT_RXBUSY	0x04
#define ESTAT_TXABRT	0x02
#define ESTAT_CLKRDY	0x01
// ENC28J60 ECON2 Register Bit Definitions
#define ECON2_AUTOINC	0x80
#define ECON2_PKTDEC	0x40
#define ECON2_PWRSV		0x20
#define ECON2_VRPS		0x08
// ENC28J60 ECON1 Register Bit Definitions
#define ECON1_TXRST		0x80
#define	ECON1_RXRST		0x40
#define ECON1_DMAST		0x20
#define ECON1_CSUMEN	0x10
#define ECON1_TXRTS		0x08
#define	ECON1_RXEN		0x04
#define ECON1_BSEL1		0x02
#define ECON1_BSEL0		0x01
// ENC28J60 MACON1 Register Bit Definitions
#define MACON1_TXPAUS	0x08
#define MACON1_RXPAUS	0x04
#define MACON1_PASSALL	0x02
#define MACON1_MARXEN	0x01
// ENC28J60 MACON3 Register Bit Definitions
#define MACON3_PADCFG2	0x80
#define MACON3_PADCFG1	0x40
#define MACON3_PADCFG0	0x20
#define MACON3_TXCRCEN	0x10
#define MACON3_PHDREN	0x08
#define MACON3_HFRMEN	0x04
#define MACON3_FRMLNEN	0x02
#define MACON3_FULDPX	0x01
// ENC28J60 MACON4 Register Bit Definitions
#define MACON4_DEFER	0x40
#define MACON4_BPEN		0x20
#define MACON4_NOBKOFF	0x10
// ENC28J60 MICMD Register Bit Definitions
#define MICMD_MIISCAN	0x02
#define MICMD_MIIRD		0x01
// ENC28J60 MISTAT Register Bit Definitions
#define MISTAT_NVALID	0x04
#define MISTAT_SCAN		0x02
#define MISTAT_BUSY		0x01
// ENC28J60 PHY PHCON1 Register Bit Definitions
#define	PHCON1_PRST		0x8000
#define	PHCON1_PLOOPBK	0x4000
#define	PHCON1_PPWRSV	0x0800
#define	PHCON1_PDPXMD	0x0100
// ENC28J60 PHY PHSTAT1 Register Bit Definitions
#define	PHSTAT1_PFDPX	0x1000
#define	PHSTAT1_PHDPX	0x0800
#define	PHSTAT1_LLSTAT	0x0004
#define	PHSTAT1_JBSTAT	0x0002
// ENC28J60 PHY PHCON2 Register Bit Definitions
#define PHCON2_FRCLINK	0x4000
#define PHCON2_TXDIS	0x2000
#define PHCON2_JABBER	0x0400
#define PHCON2_HDLDIS	0x0100

// ENC28J60 Packet Control Byte Bit Definitions
#define PKTCTRL_PHUGEEN		0x08
#define PKTCTRL_PPADEN		0x04
#define PKTCTRL_PCRCEN		0x02
#define PKTCTRL_POVERRIDE	0x01

// SPI operation codes
#define ENC28J60_READ_CTRL_REG	0x00
#define ENC28J60_READ_BUF_MEM	0x3A
#define ENC28J60_WRITE_CTRL_REG	0x40
#define ENC28J60_WRITE_BUF_MEM	0x7A
#define ENC28J60_BIT_FIELD_SET	0x80
#define ENC28J60_BIT_FIELD_CLR	0xA0
#define ENC28J60_SOFT_RESET		0xFF

// buffer boundaries applied to internal 8K ram
// entire available packet buffer space is allocated
#define TXSTART_INIT   	0x1A00	// start TX buffer 1536 bytes from end of buffer
#define TXEND_INIT		0x1FFF	// set length of TX buffer to 1536 bytes
#define RXSTART_INIT   	0x0000	// receive buffer gets the rest
#define RXSTOP_INIT    	0x19FF	// receive buffer gets the rest

#ifdef UIP_CONF_BUFFER_SIZE
#define MAX_FRAMELEN UIP_CONF_BUFFER_SIZE
#else
#define	MAX_FRAMELEN	1518	// maximum ethernet frame length
#endif

// Ethernet constants
#define ETHERNET_MIN_PACKET_LENGTH	0x3C
//#define ETHERNET_HEADER_LENGTH		0x0E

// MAC address for this interface
#ifdef UIP_ETHADDR0
#define ENC28J60_MAC0 UIP_ETHADDR0
#else
#define ENC28J60_MAC0 0x00
#endif

#ifdef UIP_ETHADDR1
#define ENC28J60_MAC1 UIP_ETHADDR1
#else
#define ENC28J60_MAC1 0xbd
#endif

#ifdef UIP_ETHADDR2
#define ENC28J60_MAC2 UIP_ETHADDR2
#else
#define ENC28J60_MAC2 0x3b
#endif

#ifdef UIP_ETHADDR3
#define ENC28J60_MAC3 UIP_ETHADDR3
#else
#define ENC28J60_MAC3 0x33
#endif

#ifdef UIP_ETHADDR4
#define ENC28J60_MAC4 UIP_ETHADDR4
#else
#define ENC28J60_MAC4 0x05
#endif

#ifdef UIP_ETHADDR5
#define ENC28J60_MAC5 UIP_ETHADDR5
#else
#define ENC28J60_MAC5 0x77
#endif
// functions

// setup ports for I/O

//void nicSetMacAddress(unsigned char *macaddr);
////! do a ENC28J60 read operation
unsigned char enc28j60ReadOp(unsigned char op, unsigned char address);
//! do a ENC28J60 write operation
void enc28j60WriteOp(unsigned char op, unsigned char address, unsigned char data);
//! read the packet buffer memory
void enc28j60ReadBuffer(unsigned int len, unsigned char* data);
//! write the packet buffer memory
void enc28j60WriteBuffer(unsigned int len, unsigned char* data);
//! set the register bank for register at address
void enc28j60SetBank(unsigned char address);
//! read ax88796 register
unsigned char enc28j60Read(unsigned char address);
//! write ax88796 register
void enc28j60Write(unsigned char address, unsigned char data);
//! read a PHY register
unsigned int enc28j60PhyRead(unsigned char address);
//! write a PHY register
void enc28j60PhyWrite(unsigned char address, unsigned int data);

//! initialize the Ethernet interface for transmit/receive
int enc28j60_init(void);

//! Packet transmit function.
/// Sends a packet on the network.  It is assumed that the packet is headed by a valid ethernet header.
/// \param len		Length of packet in bytes.
/// \param packet	Pointer to packet data.
void enc28j60PacketSend(unsigned int len, unsigned char* packet);

//! Packet receive function.
/// Gets a packet from the network receive buffer, if one is available.
/// The packet will by headed by an ethernet header.
/// \param	maxlen	The maximum acceptable length of a retrieved packet.
/// \param	packet	Pointer where packet data should be stored.
/// \return Packet length in bytes if a packet was retrieved, zero otherwise.
unsigned int enc28j60PacketReceive(unsigned int maxlen, unsigned char* packet);

int enc28j60_pending_packet();

unsigned long int enc28j60BlinkLeds(unsigned long int interval, unsigned char times);

void read_TSV(unsigned char *tsv);
// Compatibility function wrappers

void enc28j60_send(void);
u16_t enc28j60_poll(void);

#define MIN(a,b)			((a<b)?(a):(b))

#endif /* __ENC28J60_H__ */
