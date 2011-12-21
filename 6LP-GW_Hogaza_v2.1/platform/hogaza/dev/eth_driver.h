#ifndef ETH_DRIVER_H_
#define ETH_DRIVER_H_

#include "sys/process.h"

/* Driver state */
typedef enum {
	ETH_DRIVER_ON, ETH_DRIVER_OFF
} eth_driver_state_t;

/**
 * The structure of a device driver for an ethernet controller in Contiki.
 */
typedef struct {
	
	/** Initialize the driver */
  void (* init)(void);
  
  /** Send a packet. */
  void (* send)(const void *payload, unsigned short payload_len);

  /** Read a received packet into a buffer. */
  int (* read)(const void *buf, unsigned short buf_len);

    /** Check if the radio driver has just received a packet */
  int (* pending_packet)(void);

  /** Turn the driver on. */
  void (* on)(void);

  /** Turn the driver off. */
  void (* off)(void);
} eth_driver_t;


const extern eth_driver_t eth_driver;

PROCESS_NAME(eth_driver_process);

#endif /*ETH_DRIVER_H_*/
