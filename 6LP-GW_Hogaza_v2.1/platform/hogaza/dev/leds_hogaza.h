/**
 * The LED driver defines a number of macros allowing LED control.
 * There is a LED on the platform in pin 5.0.
 * 
 */

/**
 * \file
 * \brief LED driver.
 * \author Luis Maqueda <lc.maqueda@gmail.com>
 * \date May 2010
 */

#ifndef _LEDS_H
#define _LEDS_H

#include <msp430f5435a.h>

#define LEDS_OUT   P6OUT

#define LED_GREEN     	(1 << 5) // pin 6.5
#define LED_YELLOW     	(1 << 6) // pin 6.6
#define LED_RED     		(1 << 7) // pin 6.7

void led_on(unsigned char led);
void led_off(unsigned char led); 
void led_toggle(unsigned char led);
void leds_init(void);

#endif /* _LEDS_H */
