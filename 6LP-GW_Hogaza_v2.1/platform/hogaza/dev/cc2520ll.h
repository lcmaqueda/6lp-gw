

#ifndef CC2520_H_
#define CC2520_H_

#include <msp430f5435a.h>
#include "dev/hal_cc2520.h"
#include "contiki.h"
#include "net/rime/rimeaddr.h"
#include "utils/ringbuf.h"


/* peripheral interface pin definitions */
#define CC2520_RESET_PIN		1
#define CC2520_VREG_EN_PIN		7
#define CC2520_INT_PIN			0

/* spi pin definitions */
#define CC2520_CS_PIN			5
#define CC2520_CLK_PIN			6
#define CC2520_SIMO_PIN			6
#define CC2520_MISO_PIN			7


/* A microsecond in msp430 cycles at 16MHz */
#define MSP430_USECOND			16
/* A milisecond in msp430 cycles at 16MHz */
#define MSP430_MSECOND			16000
/* Ring buffer length */
#define CC2520_BUF_LEN					512
/* Startup time values (in microseconds) */
#define CC2520_XOSC_MAX_STARTUP_TIME        300
#define CC2520_VREG_MAX_STARTUP_TIME        200
#define CC2520_SRXON_TO_RANDOM_READY_TIME   144

#ifndef SUCCESS
#define SUCCESS 1
#endif

#ifndef FAILED
#define FAILED  0
#endif


/***********************************************************************************
* CONSTANTS AND DEFINES
*/

// Application parameters
#define RF_CHANNEL              25      // 2.4 GHz RF channel

// BasicRF address definitions
#define PAN_ID                	0x1234

// Node's address
#ifdef UIP_LLADDR0
#define CC2520_LLADDR0	UIP_LLADDR0
#else
#define CC2520_LLADDR0 0x00
#endif

#ifdef UIP_LLADDR1
#define CC2520_LLADDR1	UIP_LLADDR1
#else
#if NODEA
#define CC2520_LLADDR1 0x0a
#else
#define CC2520_LLADDR1 0x0b
#endif // NODEA
#endif // UIP_LLADDR1

#ifdef UIP_LLADDR2
#define CC2520_LLADDR2	UIP_LLADDR2
#else
#define CC2520_LLADDR2 0x98
#endif

#ifdef UIP_LLADDR3
#define CC2520_LLADDR3	UIP_LLADDR3
#else
#define CC2520_LLADDR3 0x00
#endif

#ifdef UIP_LLADDR4
#define CC2520_LLADDR4	UIP_LLADDR4
#else
#define CC2520_LLADDR4 0x02
#endif

#ifdef UIP_LLADDR5
#define CC2520_LLADDR5	UIP_LLADDR5
#else
#define CC2520_LLADDR5 0x32
#endif

#ifdef UIP_LLADDR6
#define CC2520_LLADDR6	UIP_LLADDR6
#else
#define CC2520_LLADDR6 0xbe
#endif

#ifdef UIP_LLADDR7
#define CC2520_LLADDR7	UIP_LLADDR7
#else
#if NODEA
#define CC2520_LLADDR7 0x07
#else
#define CC2520_LLADDR7 0x08
#endif // NODEA
#endif // UIP_LLADDR1

// Packet and packet part lengths
#define PKT_LEN_MIC                         8
#define PKT_LEN_SEC                         PKT_LEN_UNSEC + PKT_LEN_MIC
#define PKT_LEN_AUTH                        8
#define PKT_LEN_ENCR                        24

// Packet overhead ((frame control field, sequence number, PAN ID,
// destination and source) + (footer))
#define MAX_802154_PACKET_SIZE				127
// Note that the length byte itself is not included included in the packet length
#define CC2520_PACKET_OVERHEAD_SIZE       ((2 + 1 + 2 + 2 + 2) + (2))
#define CC2520_MAX_PAYLOAD_SIZE	        (127 - CC2520_PACKET_OVERHEAD_SIZE - \
    CC2520_AUX_HDR_LENGTH - CC2520_LEN_MIC)
#define CC2520_ACK_PACKET_SIZE	        5
#define CC2520_FOOTER_SIZE                2
#define CC2520_HDR_SIZE                   10

// The time it takes for the acknowledgment packet to be received after the
// data packet has been transmitted.
#define CC2520_ACK_DURATION		        (0.5 * 32 * 2 * ((4 + 1) + (1) + (2 + 1) + (2)))
#define CC2520_SYMBOL_DURATION	        (32 * 0.5)

// The length byte
#define CC2520_PLD_LEN_MASK               0x7F

// Frame control field
#define CC2520_FCF_NOACK                  0x8841
#define CC2520_FCF_ACK                    0x8861
#define CC2520_FCF_ACK_BM                 0x0020
#define CC2520_FCF_BM                     (~CC2520_FCF_ACK_BM)
#define CC2520_SEC_ENABLED_FCF_BM         0x0008

// Frame control field LSB
#define CC2520_FCF_NOACK_L                LO_UINT16(CC2520_FCF_NOACK)
#define CC2520_FCF_ACK_L                  LO_UINT16(CC2520_FCF_ACK)
#define CC2520_FCF_ACK_BM_L               LO_UINT16(CC2520_FCF_ACK_BM)
#define CC2520_FCF_BM_L                   LO_UINT16(CC2520_FCF_BM)
#define CC2520_SEC_ENABLED_FCF_BM_L       LO_UINT16(CC2520_SEC_ENABLED_FCF_BM)

// Auxiliary Security header
#define CC2520_AUX_HDR_LENGTH             5
#define CC2520_LEN_AUTH                   CC2520_PACKET_OVERHEAD_SIZE + \
    CC2520_AUX_HDR_LENGTH - CC2520_FOOTER_SIZE
#define CC2520_SECURITY_M                 2
#define CC2520_LEN_MIC                    8

// Footer
#define CC2520_CRC_OK_BM                  0x80

// IEEE 802.15.4 defined constants (2.4 GHz logical channels)
#define MIN_CHANNEL 				        11    // 2405 MHz
#define MAX_CHANNEL                         26    // 2480 MHz
#define CHANNEL_SPACING                     5     // MHz

#define min(x,y)	x<y ? x : y
/* Type definitions */

typedef struct {
    u8_t  reg;
    u8_t  val;
} regVal_t;

typedef struct {
    u16_t  panId;
    u8_t  channel;
    u8_t  ackRequest;
} cc2520ll_cfg_t;

// The receive struct
typedef struct {
    u8_t  seqNumber;
    u16_t  srcAddr;
    u16_t  srcPanId;
    s8_t length;
    u8_t * pPayload;
    u8_t  ackRequest;
    s8_t rssi;
    volatile u8_t  isReady;
    u8_t  status;
} cc2520ll_rxInfo_t;

// Tx state
typedef struct {
    u8_t  txSeqNumber;
    volatile u8_t  ackReceived;
    u8_t  receiveOn;
    u32_t  frameCounter;
} cc2520ll_rxState_t;

// Basic RF packet header (IEEE 802.15.4)
typedef struct {
    u8_t    packetLength;
    u8_t    fcf0;           // Frame control field LSB
    u8_t    fcf1;           // Frame control field MSB
    u8_t    seqNumber;
} cc2520ll_packetHdr_t;

/* External functions */

u16_t cc2520ll_init();
u16_t cc2520ll_prepare(const void *packet, unsigned short len);
u16_t cc2520ll_transmit(void);
u16_t cc2520ll_packetSend(const void* packet, unsigned short len);
u16_t cc2520ll_packetReceive(u8_t * packet, u8_t  maxlen);
u16_t cc2520ll_pending_packet(void);
void cc2520ll_receiveOn(void);
void cc2520ll_receiveOff(void);
void cc2520ll_disableRxInterrupt(void);
void cc2520ll_enableRxInterrupt(void);

// Interrupt handler routines
void cc2520ll_packetReceivedISR(void);

#endif /*CC2520_H_*/
