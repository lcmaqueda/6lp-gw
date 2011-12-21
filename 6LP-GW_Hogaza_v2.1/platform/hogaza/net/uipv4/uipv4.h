
/**
 * \addtogroup uip
 * @{
 */

/**
 * \file
 * Header file for the uIP TCP/IP stack.
 * \author  Adam Dunkels <adam@dunkels.com>
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 *
 * The uIP TCP/IP stack header file contains definitions for a number
 * of C macros that are used by uIP programs as well as internal uIP
 * structures, TCP/IP header structures and function declarations.
 *
 */

/*
 * Copyright (c) 2001-2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: uip.h,v 1.35 2010/10/19 18:29:04 adamdunkels Exp $
 *
 */

#ifndef __UIPV4_H__
#define __UIPV4_H__

#include "net/uipv4/uipv4opt.h"

/**
 * Representation of an IP address.
 *
 */
typedef union uip_ip4addr_t {
  u8_t  u8[4];			/* Initializer, must come first!!! */
  u16_t u16[2];
} uip_ip4addr_t;

/*---------------------------------------------------------------------------*/

#include "net/uipv4/tcpipv4.h"
#include "net/tcpip.h"

/*---------------------------------------------------------------------------*/
/* First, the functions that should be called from the
 * system. Initialization, the periodic timer, and incoming packets are
 * handled by the following three functions.
 */
/**
 * \defgroup uipconffunc uIP configuration functions
 * @{
 *
 * The uIP configuration functions are used for setting run-time
 * parameters in uIP such as IP addresses.
 */

/**
 * Set the IP address of this host.
 *
 * The IP address is represented as a 4-byte array where the first
 * octet of the IP address is put in the first member of the 4-byte
 * array.
 *
 * Example:
 \code

 uip_ip4addr_t addr;

 uip_ipaddr(&addr, 192,168,1,2);
 uip_sethostaddr(&addr);
 
 \endcode
 * \param addr A pointer to an IP address of type uip_ip4addr_t;
 *
 * \sa uip_ipaddr()
 *
 * \hideinitializer
 */
#define uipv4_sethostaddr(addr) uipv4_ipaddr_copy(&uipv4_hostaddr, (addr))

/**
 * Get the IP address of this host.
 *
 * The IP address is represented as a 4-byte array where the first
 * octet of the IP address is put in the first member of the 4-byte
 * array.
 *
 * Example:
 \code
 uip_ip4addr_t hostaddr;

 uip_gethostaddr(&hostaddr);
 \endcode
 * \param addr A pointer to a uip_ip4addr_t variable that will be
 * filled in with the currently configured IP address.
 *
 * \hideinitializer
 */
#define uipv4_gethostaddr(addr) uipv4_ipaddr_copy((addr), &uipv4_hostaddr)

/**
 * Set the default router's IP address.
 *
 * \param addr A pointer to a uip_ip4addr_t variable containing the IP
 * address of the default router.
 *
 * \sa uip_ipaddr()
 *
 * \hideinitializer
 */
#define uipv4_setdraddr(addr) uipv4_ipaddr_copy(&uipv4_draddr, (addr))

/**
 * Set the netmask.
 *
 * \param addr A pointer to a uip_ip4addr_t variable containing the IP
 * address of the netmask.
 *
 * \sa uip_ipaddr()
 *
 * \hideinitializer
 */
#define uipv4_setnetmask(addr) uipv4_ipaddr_copy(&uipv4_netmask, (addr))


/**
 * Get the default router's IP address.
 *
 * \param addr A pointer to a uip_ip4addr_t variable that will be
 * filled in with the IP address of the default router.
 *
 * \hideinitializer
 */
#define uipv4_getdraddr(addr) uipv4_ipaddr_copy((addr), &uipv4_draddr)

/**
 * Get the netmask.
 *
 * \param addr A pointer to a uip_ip4addr_t variable that will be
 * filled in with the value of the netmask.
 *
 * \hideinitializer
 */
#define uipv4_getnetmask(addr) uipv4_ipaddr_copy((addr), &uipv4_netmask)

/** @} */

/**
 * \defgroup uipinit uIP initialization functions
 * @{
 *
 * The uIP initialization functions are used for booting uIP.
 */

/**
 * uIP initialization function.
 *
 * This function should be called at boot up to initilize the uIP
 * TCP/IP stack.
 */
void uipv4_init(void);

/**
 * uIP initialization function.
 *
 * This function may be used at boot time to set the initial ip_id.
 */
void uipv4_setipid(u16_t id);

/** @} */

/**
 * \defgroup uipdevfunc uIP device driver functions
 * @{
 *
 * These functions are used by a network device driver for interacting
 * with uIP.
 */

/**
 * Process an incoming packet.
 *
 * This function should be called when the device driver has received
 * a packet from the network. The packet from the device driver must
 * be present in the uip_buf buffer, and the length of the packet
 * should be placed in the uip_len variable.
 *
 * When the function returns, there may be an outbound packet placed
 * in the uip_buf packet buffer. If so, the uip_len variable is set to
 * the length of the packet. If no packet is to be sent out, the
 * uip_len variable is set to 0.
 *
 * The usual way of calling the function is presented by the source
 * code below.
 \code
 uip_len = devicedriver_poll();
 if(uip_len > 0) {
 uip_input();
 if(uip_len > 0) {
 devicedriver_send();
 }
 }
 \endcode
 *
 * \note If you are writing a uIP device driver that needs ARP
 * (Address Resolution Protocol), e.g., when running uIP over
 * Ethernet, you will need to call the uIP ARP code before calling
 * this function:
 \code
 #define BUF ((struct uip_eth_hdr *)&uip_buf[0])
 uip_len = ethernet_devicedrver_poll();
 if(uip_len > 0) {
 if(BUF->type == UIP_HTONS(UIP_ETHTYPE_IP)) {
 uipv4_arp_ipin();
 uip_input();
 if(uip_len > 0) {
 uipv4_arp_out();
 ethernet_devicedriver_send();
 }
 } else if(BUF->type == UIP_HTONS(UIP_ETHTYPE_ARP)) {
 uipv4_arp_arpin();
 if(uip_len > 0) {
 ethernet_devicedriver_send();
 }
 }
 \endcode
 *
 * \hideinitializer
 */
#define uipv4_input()        uipv4_process(UIP_DATA)


/**
 * Periodic processing for a connection identified by its number.
 *
 * This function does the necessary periodic processing (timers,
 * polling) for a uIP TCP conneciton, and should be called when the
 * periodic uIP timer goes off. It should be called for every
 * connection, regardless of whether they are open of closed.
 *
 * When the function returns, it may have an outbound packet waiting
 * for service in the uIP packet buffer, and if so the uip_len
 * variable is set to a value larger than zero. The device driver
 * should be called to send out the packet.
 *
 * The usual way of calling the function is through a for() loop like
 * this:
 \code
 for(i = 0; i < UIPV4_CONNS; ++i) {
 uip_periodic(i);
 if(uip_len > 0) {
 devicedriver_send();
 }
 }
 \endcode
 *
 * \note If you are writing a uIP device driver that needs ARP
 * (Address Resolution Protocol), e.g., when running uIP over
 * Ethernet, you will need to call the uipv4_arp_out() function before
 * calling the device driver:
 \code
 for(i = 0; i < UIPV4_CONNS; ++i) {
 uip_periodic(i);
 if(uip_len > 0) {
 uipv4_arp_out();
 ethernet_devicedriver_send();
 }
 }
 \endcode
 *
 * \param conn The number of the connection which is to be periodically polled.
 *
 * \hideinitializer
 */
#if UIPV4_TCP
#define uipv4_periodic(conn) do { uipv4_conn = &uipv4_conns[conn];    \
    uipv4_process(UIP_TIMER); } while (0)

/**
 *
 *
 */
#define uipv4_conn_active(conn) (uipv4_conns[conn].tcpstateflags != UIP_CLOSED)

/**
 * Perform periodic processing for a connection identified by a pointer
 * to its structure.
 *
 * Same as uip_periodic() but takes a pointer to the actual uipv4_conn
 * struct instead of an integer as its argument. This function can be
 * used to force periodic processing of a specific connection.
 *
 * \param conn A pointer to the uipv4_conn struct for the connection to
 * be processed.
 *
 * \hideinitializer
 */
#define uipv4_periodic_conn(conn) do { uipv4_conn = conn;   \
    uipv4_process(UIP_TIMER); } while (0)

/**
 * Request that a particular connection should be polled.
 *
 * Similar to uip_periodic_conn() but does not perform any timer
 * processing. The application is polled for new data.
 *
 * \param conn A pointer to the uipv4_conn struct for the connection to
 * be processed.
 *
 * \hideinitializer
 */
#define uipv4_poll_conn(conn) do { uipv4_conn = conn;       \
    uipv4_process(UIP_POLL_REQUEST); } while (0)

#endif /* UIPV4_TCP */

#if UIPV4_UDP
/**
 * Periodic processing for a UDP connection identified by its number.
 *
 * This function is essentially the same as uip_periodic(), but for
 * UDP connections. It is called in a similar fashion as the
 * uip_periodic() function:
 \code
 for(i = 0; i < UIPV4_UDP_CONNS; i++) {
 uip_udp_periodic(i);
 if(uip_len > 0) {
 devicedriver_send();
 }
 }
 \endcode
 *
 * \note As for the uip_periodic() function, special care has to be
 * taken when using uIP together with ARP and Ethernet:
 \code
 for(i = 0; i < UIPV4_UDP_CONNS; i++) {
 uip_udp_periodic(i);
 if(uip_len > 0) {
 uipv4_arp_out();
 ethernet_devicedriver_send();
 }
 }
 \endcode
 *
 * \param conn The number of the UDP connection to be processed.
 *
 * \hideinitializer
 */
#define uipv4_udp_periodic(conn) do { uipv4_udp_conn = &uipv4_udp_conn[conn]; \
    uipv4_process(UIP_UDP_TIMER); } while(0)

/**
 * Periodic processing for a UDP connection identified by a pointer to
 * its structure.
 *
 * Same as uip_udp_periodic() but takes a pointer to the actual
 * uipv4_conn struct instead of an integer as its argument. This
 * function can be used to force periodic processing of a specific
 * connection.
 *
 * \param conn A pointer to the uipv4_udp_conn struct for the connection
 * to be processed.
 *
 * \hideinitializer
 */
#define uipv4_udp_periodic_conn(conn) do { uipv4_udp_conn = conn;   \
    uipv4_process(UIP_UDP_TIMER); } while(0)
#endif /* UIPV4_UDP */

/** \brief Abandon the reassembly of the current packet */
void uipv4_reass_over(void);

/** @} */

/*---------------------------------------------------------------------------*/
/* Functions that are used by the uIP application program. Opening and
 * closing connections, sending and receiving data, etc. is all
 * handled by the functions below.
 */
/**
 * \defgroup uipappfunc uIP application functions
 * @{
 *
 * Functions used by an application running of top of uIP.
 */

/**
 * Start listening to the specified port.
 *
 * \note Since this function expects the port number in network byte
 * order, a conversion using UIP_HTONS() or uip_htons() is necessary.
 *
 \code
 uipv4_listen(UIP_HTONS(80));
 \endcode
 *
 * \param port A 16-bit port number in network byte order.
 */
void uipv4_listen(u16_t port);

/**
 * Stop listening to the specified port.
 *
 * \note Since this function expects the port number in network byte
 * order, a conversion using UIP_HTONS() or uip_htons() is necessary.
 *
 \code
 uipv4_unlisten(UIP_HTONS(80));
 \endcode
 *
 * \param port A 16-bit port number in network byte order.
 */
void uipv4_unlisten(u16_t port);

/**
 * Connect to a remote host using TCP.
 *
 * This function is used to start a new connection to the specified
 * port on the specified host. It allocates a new connection identifier,
 * sets the connection to the SYN_SENT state and sets the
 * retransmission timer to 0. This will cause a TCP SYN segment to be
 * sent out the next time this connection is periodically processed,
 * which usually is done within 0.5 seconds after the call to
 * uipv4_connect().
 *
 * \note This function is available only if support for active open
 * has been configured by defining UIPV4_ACTIVE_OPEN to 1 in uipopt.h.
 *
 * \note Since this function requires the port number to be in network
 * byte order, a conversion using UIP_HTONS() or uip_htons() is necessary.
 *
 \code
 uip_ip4addr_t ipaddr;

 uip_ipaddr(&ipaddr, 192,168,1,2);
 uipv4_connect(&ipaddr, UIP_HTONS(80));
 \endcode
 *
 * \param ripaddr The IP address of the remote host.
 *
 * \param port A 16-bit port number in network byte order.
 *
 * \return A pointer to the uIP connection identifier for the new connection,
 * or NULL if no connection could be allocated.
 *
 */
struct uipv4_conn *uipv4_connect(uip_ip4addr_t *ripaddr, u16_t port);



/**
 * \internal
 *
 * Check if a connection has outstanding (i.e., unacknowledged) data.
 *
 * \param conn A pointer to the uipv4_conn structure for the connection.
 *
 * \hideinitializer
 */
#define uipv4_outstanding(conn) ((conn)->len)

/**
 * Send data on the current connection.
 *
 * This function is used to send out a single segment of TCP
 * data. Only applications that have been invoked by uIP for event
 * processing can send data.
 *
 * The amount of data that actually is sent out after a call to this
 * function is determined by the maximum amount of data TCP allows. uIP
 * will automatically crop the data so that only the appropriate
 * amount of data is sent. The function uip_mss() can be used to query
 * uIP for the amount of data that actually will be sent.
 *
 * \note This function does not guarantee that the sent data will
 * arrive at the destination. If the data is lost in the network, the
 * application will be invoked with the uip_rexmit() event being
 * set. The application will then have to resend the data using this
 * function.
 *
 * \param data A pointer to the data which is to be sent.
 *
 * \param len The maximum amount of data bytes to be sent.
 *
 * \hideinitializer
 */
CCIF void uipv4_send(const void *data, int len);

/**
 * The length of any incoming data that is currently available (if available)
 * in the uip_appdata buffer.
 *
 * The test function uip_data() must first be used to check if there
 * is any data available at all.
 *
 * \hideinitializer
 */
/*void uip_datalen(void);*/
#define uipv4_datalen()       uip_len

/**
 * The length of any out-of-band data (urgent data) that has arrived
 * on the connection.
 *
 * \note The configuration parameter UIP_URGDATA must be set for this
 * function to be enabled.
 *
 * \hideinitializer
 */
#define uipv4_urgdatalen()    uip_urglen

/**
 * Close the current connection.
 *
 * This function will close the current connection in a nice way.
 *
 * \hideinitializer
 */
#define uipv4_close()         (uipv4_flags = UIP_CLOSE)

/**
 * Abort the current connection.
 *
 * This function will abort (reset) the current connection, and is
 * usually used when an error has occurred that prevents using the
 * uip_close() function.
 *
 * \hideinitializer
 */
#define uipv4_abort()         (uipv4_flags = UIP_ABORT)

/**
 * Tell the sending host to stop sending data.
 *
 * This function will close our receiver's window so that we stop
 * receiving data for the current connection.
 *
 * \hideinitializer
 */
#define uipv4_stop()          (uip_conn->tcpstateflags |= UIP_STOPPED)

/**
 * Find out if the current connection has been previously stopped with
 * uip_stop().
 *
 * \hideinitializer
 */
#define uipv4_stopped(conn)   ((conn)->tcpstateflags & UIP_STOPPED)

/**
 * Restart the current connection, if is has previously been stopped
 * with uip_stop().
 *
 * This function will open the receiver's window again so that we
 * start receiving data for the current connection.
 *
 * \hideinitializer
 */
#define uipv4_restart()         do { uipv4_flags |= UIP_NEWDATA;    \
    uipv4_conn->tcpstateflags &= ~UIP_STOPPED;                    \
  } while(0)


/* uIP tests that can be made to determine in what state the current
   connection is, and what the application function should do. */

/**
 * Is the current connection a UDP connection?
 *
 * This function checks whether the current connection is a UDP connection.
 *
 * \hideinitializer
 *
 */
#define uipv4_udpconnection() (uipv4_conn == NULL)

/**
 * Is new incoming data available?
 *
 * Will reduce to non-zero if there is new data for the application
 * present at the uip_appdata pointer. The size of the data is
 * available through the uip_len variable.
 *
 * \hideinitializer
 */
#define uipv4_newdata()   (uipv4_flags & UIP_NEWDATA)

/**
 * Has previously sent data been acknowledged?
 *
 * Will reduce to non-zero if the previously sent data has been
 * acknowledged by the remote host. This means that the application
 * can send new data.
 *
 * \hideinitializer
 */
#define uipv4_acked()   (uipv4_flags & UIP_ACKDATA)

/**
 * Has the connection just been connected?
 *
 * Reduces to non-zero if the current connection has been connected to
 * a remote host. This will happen both if the connection has been
 * actively opened (with uipv4_connect()) or passively opened (with
 * uipv4_listen()).
 *
 * \hideinitializer
 */
#define uipv4_connected() (uipv4_flags & UIP_CONNECTED)

/**
 * Has the connection been closed by the other end?
 *
 * Is non-zero if the connection has been closed by the remote
 * host. The application may then do the necessary clean-ups.
 *
 * \hideinitializer
 */
#define uipv4_closed()    (uipv4_flags & UIP_CLOSE)

/**
 * Has the connection been aborted by the other end?
 *
 * Non-zero if the current connection has been aborted (reset) by the
 * remote host.
 *
 * \hideinitializer
 */
#define uipv4_aborted()    (uipv4_flags & UIP_ABORT)

/**
 * Has the connection timed out?
 *
 * Non-zero if the current connection has been aborted due to too many
 * retransmissions.
 *
 * \hideinitializer
 */
#define uipv4_timedout()    (uipv4_flags & UIP_TIMEDOUT)

/**
 * Do we need to retransmit previously data?
 *
 * Reduces to non-zero if the previously sent data has been lost in
 * the network, and the application should retransmit it. The
 * application should send the exact same data as it did the last
 * time, using the uipv4_send() function.
 *
 * \hideinitializer
 */
#define uipv4_rexmit()     (uipv4_flags & UIP_REXMIT)

/**
 * Is the connection being polled by uIP?
 *
 * Is non-zero if the reason the application is invoked is that the
 * current connection has been idle for a while and should be
 * polled.
 *
 * The polling event can be used for sending data without having to
 * wait for the remote host to send data.
 *
 * \hideinitializer
 */
#define uipv4_poll()       (uipv4_flags & UIP_POLL)

/**
 * Get the initial maximum segment size (MSS) of the current
 * connection.
 *
 * \hideinitializer
 */
#define uipv4_initialmss()             (uipv4_conn->initialmss)

/**
 * Get the current maximum segment size that can be sent on the current
 * connection.
 *
 * The current maximum segment size that can be sent on the
 * connection is computed from the receiver's window and the MSS of
 * the connection (which also is available by calling
 * uip_initialmss()).
 *
 * \hideinitializer
 */
#define uipv4_mss()             (uipv4_conn->mss)

/**
 * Set up a new UDP connection.
 *
 * This function sets up a new UDP connection. The function will
 * automatically allocate an unused local port for the new
 * connection. However, another port can be chosen by using the
 * uip_udp_bind() call, after the uipv4_udp_new() function has been
 * called.
 *
 * Example:
 \code
 uip_ip4addr_t addr;
 struct uipv4_udp_conn *c;
 
 uip_ipaddr(&addr, 192,168,2,1);
 c = uipv4_udp_new(&addr, UIP_HTONS(12345));
 if(c != NULL) {
 uip_udp_bind(c, UIP_HTONS(12344));
 }
 \endcode
 * \param ripaddr The IP address of the remote host.
 *
 * \param rport The remote port number in network byte order.
 *
 * \return The uipv4_udp_conn structure for the new connection or NULL
 * if no connection could be allocated.
 */
struct uipv4_udp_conn *uipv4_udp_new(const uip_ip4addr_t *ripaddr, u16_t rport);

/**
 * Removed a UDP connection.
 *
 * \param conn A pointer to the uipv4_udp_conn structure for the connection.
 *
 * \hideinitializer
 */
#define uipv4_udp_remove(conn) (conn)->lport = 0

/**
 * Bind a UDP connection to a local port.
 *
 * \param conn A pointer to the uipv4_udp_conn structure for the
 * connection.
 *
 * \param port The local port number, in network byte order.
 *
 * \hideinitializer
 */
#define uipv4_udp_bind(conn, port) (conn)->lport = port

/**
 * Send a UDP datagram of length len on the current connection.
 *
 * This function can only be called in response to a UDP event (poll
 * or newdata). The data must be present in the uip_buf buffer, at the
 * place pointed to by the uip_appdata pointer.
 *
 * \param len The length of the data in the uip_buf buffer.
 *
 * \hideinitializer
 */
#define uipv4_udp_send(len) uipv4_send((char *)uip_appdata, len)

/** @} */

/* uIP convenience and converting functions. */

/**
 * \defgroup uipconvfunc uIP conversion functions
 * @{
 *
 * These functions can be used for converting between different data
 * formats used by uIP.
 */
 
/**
 * Convert an IP address to four bytes separated by commas.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr;
 printf("ipaddr=%d.%d.%d.%d\n", uip_ip4addr_to_quad(&ipaddr));
 \endcode
 *
 * \param a A pointer to a uip_ip4addr_t.
 * \hideinitializer
 */
#define uipv4_ipaddr_to_quad(a) (a)->u8[0],(a)->u8[1],(a)->u8[2],(a)->u8[3]

/**
 * Construct an IP address from four bytes.
 *
 * This function constructs an IP address of the type that uIP handles
 * internally from four bytes. The function is handy for specifying IP
 * addresses to use with e.g. the uipv4_connect() function.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr;
 struct uipv4_conn *c;
 
 uip_ipaddr(&ipaddr, 192,168,1,2);
 c = uipv4_connect(&ipaddr, UIP_HTONS(80));
 \endcode
 *
 * \param addr A pointer to a uip_ip4addr_t variable that will be
 * filled in with the IP address.
 *
 * \param addr0 The first octet of the IP address.
 * \param addr1 The second octet of the IP address.
 * \param addr2 The third octet of the IP address.
 * \param addr3 The forth octet of the IP address.
 *
 * \hideinitializer
 */
#define uip_ip4addr(addr, addr0,addr1,addr2,addr3) do {  \
    (addr)->u8[0] = addr0;                              \
    (addr)->u8[1] = addr1;                              \
    (addr)->u8[2] = addr2;                              \
    (addr)->u8[3] = addr3;                              \
  } while(0)

/**
 * Copy an IP address to another IP address.
 *
 * Copies an IP address from one place to another.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr1, ipaddr2;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uipv4_ipaddr_copy(&ipaddr2, &ipaddr1);
 \endcode
 *
 * \param dest The destination for the copy.
 * \param src The source from where to copy.
 *
 * \hideinitializer
 */
#ifndef uipv4_ipaddr_copy
#define uipv4_ipaddr_copy(dest, src) (*(dest) = *(src))
#endif

/**
 * Compare two IP addresses
 *
 * Compares two IP addresses.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr1, ipaddr2;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 if(uipv4_ipaddr_cmp(&ipaddr2, &ipaddr1)) {
 printf("They are the same");
 }
 \endcode
 *
 * \param addr1 The first IP address.
 * \param addr2 The second IP address.
 *
 * \hideinitializer
 */
#define uipv4_ipaddr_cmp(addr1, addr2) ((addr1)->u16[0] == (addr2)->u16[0] && \
				      (addr1)->u16[1] == (addr2)->u16[1])

/**
 * Compare two IP addresses with netmasks
 *
 * Compares two IP addresses with netmasks. The masks are used to mask
 * out the bits that are to be compared.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr1, ipaddr2, mask;

 uip_ipaddr(&mask, 255,255,255,0);
 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uip_ipaddr(&ipaddr2, 192,16,1,3);
 if(uip_ipaddr_maskcmp(&ipaddr1, &ipaddr2, &mask)) {
 printf("They are the same");
 }
 \endcode
 *
 * \param addr1 The first IP address.
 * \param addr2 The second IP address.
 * \param mask The netmask.
 *
 * \hideinitializer
 */
#define uipv4_ipaddr_maskcmp(addr1, addr2, mask)          \
  (((((u16_t *)addr1)[0] & ((u16_t *)mask)[0]) ==       \
    (((u16_t *)addr2)[0] & ((u16_t *)mask)[0])) &&      \
   ((((u16_t *)addr1)[1] & ((u16_t *)mask)[1]) ==       \
    (((u16_t *)addr2)[1] & ((u16_t *)mask)[1])))


/**
 * Check if an address is a broadcast address for a network.
 *
 * Checks if an address is the broadcast address for a network. The
 * network is defined by an IP address that is on the network and the
 * network's netmask.
 *
 * \param addr The IP address.
 * \param netaddr The network's IP address.
 * \param netmask The network's netmask.
 *
 * \hideinitializer
 */
/*#define uip_ipaddr_isbroadcast(addr, netaddr, netmask)
  ((uip_ip4addr_t *)(addr)).u16 & ((uip_ip4addr_t *)(addr)).u16*/



/**
 * Mask out the network part of an IP address.
 *
 * Masks out the network part of an IP address, given the address and
 * the netmask.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr1, ipaddr2, netmask;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uip_ipaddr(&netmask, 255,255,255,0);
 uip_ipaddr_mask(&ipaddr2, &ipaddr1, &netmask);
 \endcode
 *
 * In the example above, the variable "ipaddr2" will contain the IP
 * address 192.168.1.0.
 *
 * \param dest Where the result is to be placed.
 * \param src The IP address.
 * \param mask The netmask.
 *
 * \hideinitializer
 */
#define uipv4_ipaddr_mask(dest, src, mask) do {                           \
    ((u16_t *)dest)[0] = ((u16_t *)src)[0] & ((u16_t *)mask)[0];        \
    ((u16_t *)dest)[1] = ((u16_t *)src)[1] & ((u16_t *)mask)[1];        \
  } while(0)

/**
 * Pick the first octet of an IP address.
 *
 * Picks out the first octet of an IP address.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr;
 u8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr1(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 1.
 *
 * \hideinitializer
 */
#define uipv4_ipaddr1(addr) ((addr)->u8[0])

/**
 * Pick the second octet of an IP address.
 *
 * Picks out the second octet of an IP address.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr;
 u8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr2(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 2.
 *
 * \hideinitializer
 */
#define uipv4_ipaddr2(addr) ((addr)->u8[1])

/**
 * Pick the third octet of an IP address.
 *
 * Picks out the third octet of an IP address.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr;
 u8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr3(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 3.
 *
 * \hideinitializer
 */
#define uipv4_ipaddr3(addr) ((addr)->u8[2])

/**
 * Pick the fourth octet of an IP address.
 *
 * Picks out the fourth octet of an IP address.
 *
 * Example:
 \code
 uip_ip4addr_t ipaddr;
 u8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr4(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 4.
 *
 * \hideinitializer
 */
#define uipv4_ipaddr4(addr) ((addr)->u8[3])

/**
 * Representation of a uIP TCP connection (for IPv4).
 *
 * The uipv4_conn structure is used for identifying a connection. All
 * but one field in the structure are to be considered read-only by an
 * application. The only exception is the appstate field whose purpose
 * is to let the application store application-specific state (e.g.,
 * file pointers) for the connection. The type of this field is
 * configured in the "uipv4opt.h" header file.
 */
struct uipv4_conn {
  uip_ip4addr_t ripaddr;   /**< The IPv4 address of the remote host. */
  
  u16_t lport;        /**< The local TCP port, in network byte order. */
  u16_t rport;        /**< The local remote TCP port, in network byte
			 order. */
  
  u8_t rcv_nxt[4];    /**< The sequence number that we expect to
			 receive next. */
  u8_t snd_nxt[4];    /**< The sequence number that was last sent by
                         us. */
  u16_t len;          /**< Length of the data that was previously sent. */
  u16_t mss;          /**< Current maximum segment size for the
			 connection. */
  u16_t initialmss;   /**< Initial maximum segment size for the
			 connection. */
  u8_t sa;            /**< Retransmission time-out calculation state
			 variable. */
  u8_t sv;            /**< Retransmission time-out calculation state
			 variable. */
  u8_t rto;           /**< Retransmission time-out. */
  u8_t tcpstateflags; /**< TCP state and flags. */
  u8_t timer;         /**< The retransmission timer. */
  u8_t nrtx;          /**< The number of retransmissions for the last
			 segment sent. */

  /** The application state. */
  uip_tcp_appstate_t appstate;
};


/**
 * Pointer to the current TCP connection.
 *
 * The uipv4_conn pointer can be used to access the current TCP
 * connection.
 */

CCIF extern struct uipv4_conn *uipv4_conn;
#if UIPV4_TCP
/* The array containing all uIP connections. */
CCIF extern struct uipv4_conn uipv4_conns[UIPV4_CONNS];
#endif

/**
 * \addtogroup uiparch
 * @{
 */
/**
 * Representation of a uIP UDP connection.
 */
struct uipv4_udp_conn {
  uip_ip4addr_t ripaddr;   /**< The IP address of the remote peer. */
  u16_t lport;        /**< The local port number in network byte order. */
  u16_t rport;        /**< The remote port number in network byte order. */
  u8_t  ttl;          /**< Default time-to-live. */

  /** The application state. */
  uip_udp_appstate_t appstate;
};

/**
 * The current UDP connection.
 */
extern struct uipv4_udp_conn *uipv4_udp_conn;
extern struct uipv4_udp_conn uipv4_udp_conns[UIPV4_UDP_CONNS];

/**
 * The uIP TCP/IP statistics.
 *
 * This is the variable in which the uIP TCP/IP statistics are gathered.
 */

/*---------------------------------------------------------------------------*/
/* All the stuff below this point is internal to uIP and should not be
 * used directly by an application or by a device driver.
 */
/*---------------------------------------------------------------------------*/



/* u8_t uipv4_flags:
 *
 * When the application is called, uipv4_flags will contain the flags
 * that are defined in this file. Please read below for more
 * information.
 */
CCIF extern u8_t uipv4_flags;

/* The following flags may be set in the global variable uipv4_flags
   before calling the application callback. The UIP_ACKDATA,
   UIP_NEWDATA, and UIP_CLOSE flags may both be set at the same time,
   whereas the others are mutually exclusive. Note that these flags
   should *NOT* be accessed directly, but only through the uIP
   functions/macros. */

#define UIP_ACKDATA   1     /* Signifies that the outstanding data was
			       acked and the application should send
			       out new data instead of retransmitting
			       the last data. */
#define UIP_NEWDATA   2     /* Flags the fact that the peer has sent
			       us new data. */
#define UIP_REXMIT    4     /* Tells the application to retransmit the
			       data that was last sent. */
#define UIP_POLL      8     /* Used for polling the application, to
			       check if the application has data that
			       it wants to send. */
#define UIP_CLOSE     16    /* The remote host has closed the
			       connection, thus the connection has
			       gone away. Or the application signals
			       that it wants to close the
			       connection. */
#define UIP_ABORT     32    /* The remote host has aborted the
			       connection, thus the connection has
			       gone away. Or the application signals
			       that it wants to abort the
			       connection. */
#define UIP_CONNECTED 64    /* We have got a connection from a remote
                               host and have set up a new connection
                               for it, or an active connection has
                               been successfully established. */

#define UIP_TIMEDOUT  128   /* The connection has been aborted due to
			       too many retransmissions. */


/**
 * \brief process the options within a hop by hop or destination option header
 * \retval 0: nothing to send,
 * \retval 1: drop pkt
 * \retval 2: ICMP error message to send
*/
/*static u8_t
uip_ext_hdr_options_process(); */

/* uip_process(flag):
 *
 * The actual uIP function which does all the work.
 */
void uipv4_process(u8_t flag);
  
  /* The following flags are passed as an argument to the uip_process()
   function. They are used to distinguish between the two cases where
   uip_process() is called. It can be called either because we have
   incoming data that should be processed, or because the periodic
   timer has fired. These values are never used directly, but only in
   the macros defined in this file. */
 
#define UIP_DATA          1     /* Tells uIP that there is incoming
				   data in the uip_buf buffer. The
				   length of the data is stored in the
				   global variable uip_len. */
#define UIP_TIMER         2     /* Tells uIP that the periodic timer
				   has fired. */
#define UIP_POLL_REQUEST  3     /* Tells uIP that a connection should
				   be polled. */
#define UIP_UDP_SEND_CONN 4     /* Tells uIP that a UDP datagram
				   should be constructed in the
				   uip_buf buffer. */
#if UIPV4_UDP
#define UIP_UDP_TIMER     5
#endif /* UIPV4_UDP */

/* The TCP states used in the uipv4_conn->tcpstateflags. */
#define UIP_CLOSED      0
#define UIP_SYN_RCVD    1
#define UIP_SYN_SENT    2
#define UIP_ESTABLISHED 3
#define UIP_FIN_WAIT_1  4
#define UIP_FIN_WAIT_2  5
#define UIP_CLOSING     6
#define UIP_TIME_WAIT   7
#define UIP_LAST_ACK    8
#define UIP_TS_MASK     15
  
#define UIP_STOPPED      16

/* The TCP and IP headers. */
struct uipv4_tcpip_hdr {
  /* IPv4 header. */
  u8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  u16_t ipchksum;
  uip_ip4addr_t srcipaddr, destipaddr;
  
  /* TCP header. */
  u16_t srcport,
    destport;
  u8_t seqno[4],
    ackno[4],
    tcpoffset,
    flags,
    wnd[2];
  u16_t tcpchksum;
  u8_t urgp[2];
  u8_t optdata[4];
};

/* The ICMP and IP headers. */
struct uipv4_icmpip_hdr {
  /* IPv4 header. */
  u8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  u16_t ipchksum;
  uip_ip4addr_t srcipaddr, destipaddr;
  
  /* ICMP header. */
  u8_t type, icode;
  u16_t icmpchksum;
  u16_t id, seqno;
  u8_t payload[1];
};


/* The UDP and IP headers. */
struct uipv4_udpip_hdr {
  /* IP header. */
  u8_t vhl,
  	tos,
  	len[2],
  	ipid[2],
  	ipoffset[2],
  	ttl,
  	proto;
  u16_t ipchksum;
  uip_ip4addr_t srcipaddr, destipaddr;
  
  /* UDP header. */
  u16_t srcport,
  destport;
  u16_t udplen;
  u16_t udpchksum;
};

/* The IPv4 header */
struct uipv4_ip_hdr {
  u8_t vhl,
  	tos,
  	len[2],
  	ipid[2],
  	ipoffset[2],
  	ttl,
  	proto;
  u16_t ipchksum;
  uip_ip4addr_t srcipaddr, destipaddr;
};

/* Upper layer headers already defined in uip.h */
///* TCP header */
//struct uip_tcp_hdr {
//  u16_t srcport;
//  u16_t destport;
//  u8_t seqno[4];
//  u8_t ackno[4];
//  u8_t tcpoffset;
//  u8_t flags;
//  u8_t wnd[2];
//  u16_t tcpchksum;
//  u8_t urgp[2];
//  u8_t optdata[4];
//};
//
///* The ICMP headers. */
//struct uip_icmp_hdr {
//  u8_t type, icode;
//  u16_t icmpchksum;
//  u16_t id, seqno;
//};
//
//
///* The UDP headers. */
//struct uip_udp_hdr {
//  u16_t srcport;
//  u16_t destport;
//  u16_t udplen;
//  u16_t udpchksum;
//};


/**
 * The buffer size available for user data in the \ref uip_buf buffer.
 *
 * This macro holds the available size for user data in the \ref
 * uip_buf buffer. The macro is intended to be used for checking
 * bounds of available user data.
 *
 * Example:
 \code
 snprintf(uip_appdata, UIPV4_APPDATA_SIZE, "%u\n", i);
 \endcode
 *
 * \hideinitializer
 */
#define UIPV4_APPDATA_SIZE (UIP_BUFSIZE - UIP_LLH_LEN - UIPV4_TCPIP_HLEN)
#define UIPV4_APPDATA_PTR (void *)&uip_buf[UIP_LLH_LEN + UIPV4_TCPIP_HLEN]

#define UIP_PROTO_ICMP  1
#define UIP_PROTO_TCP   6
#define UIP_PROTO_UDP   17
#define UIP_PROTO_ICMP6 58

/* Header sizes. */
#define UIPV4_IPH_LEN    20    /* Size of IP header */
#define UIP_UDPH_LEN    8    /* Size of UDP header */
#define UIP_TCPH_LEN   20    /* Size of TCP header */
#ifdef UIPV4_IPH_LEN
#define UIP_ICMPH_LEN   4    /* Size of ICMP header */
#endif
#define UIPV4_IPUDPH_LEN (UIP_UDPH_LEN + UIPV4_IPH_LEN)    /* Size of IP +
                        * UDP
							   * header */
#define UIPV4_IPTCPH_LEN (UIP_TCPH_LEN + UIPV4_IPH_LEN)    /* Size of IP +
							   * TCP
							   * header */
#define UIPV4_TCPIP_HLEN UIPV4_IPTCPH_LEN
#define UIPV4_IPICMPH_LEN (UIPV4_IPH_LEN + UIP_ICMPH_LEN) /* size of ICMP
                                                         + IP header */
#define UIPV4_LLIPH_LEN (UIP_LLH_LEN + UIPV4_IPH_LEN)    /* size of L2
                                                        + IP header */


#if UIP_FIXEDADDR
CCIF extern const uip_ip4addr_t uipv4_hostaddr, uipv4_netmask, uipv4_draddr;
#else /* UIPV4_FIXEDADDR */
CCIF extern uip_ip4addr_t uipv4_hostaddr, uipv4_netmask, uipv4_draddr;
#endif /* UIPV4_FIXEDADDR */
CCIF extern const uip_ip4addr_t uipv4_broadcast_addr;
CCIF extern const uip_ip4addr_t uipv4_all_zeroes_addr;

#if UIPV4_FIXEDETHADDR
extern const struct uip_eth_addr uip_ethaddr;
#else
extern struct uip_eth_addr uip_ethaddr;
#endif

/**
 * Calculate the IP header checksum of the packet header in uip_buf.
 *
 * The IP header checksum is the Internet checksum of the 20 bytes of
 * the IP header.
 *
 * \return The IP header checksum of the IP header in the uip_buf
 * buffer.
 */
u16_t uipv4_ipchksum(void);

/**
 * Calculate the TCP checksum of the packet in uip_buf and uip_appdata.
 *
 * The TCP checksum is the Internet checksum of data contents of the
 * TCP segment, and a pseudo-header as defined in RFC793.
 *
 * \return The TCP checksum of the TCP segment in uip_buf and pointed
 * to by uip_appdata.
 */
u16_t uipv4_tcpchksum(void);

/**
 * Calculate the UDP checksum of the packet in uip_buf and uip_appdata.
 *
 * The UDP checksum is the Internet checksum of data contents of the
 * UDP segment, and a pseudo-header as defined in RFC768.
 *
 * \return The UDP checksum of the UDP segment in uip_buf and pointed
 * to by uip_appdata.
 */
u16_t uipv4_udpchksum(void);

/**
 * Calculate the ICMP checksum of the packet in uip_buf.
 *
 * \return The ICMP checksum of the ICMP packet in uip_buf
 */
u16_t uip_icmp6chksum(void);


#endif /* __UIPV4_H__ */


/** @} */
