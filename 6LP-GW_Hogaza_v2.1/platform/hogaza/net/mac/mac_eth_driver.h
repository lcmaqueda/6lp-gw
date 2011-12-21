#ifndef MAC_ETH_DRIVER_H_
#define MAC_ETH_DRIVER_H_

#include "contiki-net.h"

/**
 * The structure of an Ethernet MAC driver in Contiki.
 */
struct mac_eth_driver {
  char *name;

  /** Initialize the Ethernet MAC driver */
  void (* init)(void);

  /** Send a packet from the uip_buf buffer  */
  void (* send)(void);

  /** Callback for getting notified of incoming packet. */
  void (* input)(void);

  /** Turn the MAC layer on. */
  void (* on)(void);

  /** Turn the MAC layer off. */
  void (* off)(void);

};

extern const struct mac_eth_driver mac_eth_driver;

PROCESS_NAME(mac_eth_process);

#endif /*MAC_ETH_DRIVER_H_*/
