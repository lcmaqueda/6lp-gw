/**
 * \file		buttons.c
 *
 * \brief		Architecture-specific button functions. Platform Hogaza v1.2
 * 				contains two LEDs in pins 1.4 and 2.6.
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "contiki.h"

#include <msp430f5435a.h>

#define BUTTON1_IN				P1IN
#define BUTTON2_IN				P2IN

#define BUTTON1_PIN				4 // pin 1.4
#define BUTTON2_PIN				6 // pin 2.6

#define clear_ifg1()			P1IFG &= ~(1 << BUTTON1_PIN)
#define clear_ifg2()			P2IFG &= ~(1 << BUTTON2_PIN)
/* The maximum number of processes allowed to be registered as buttons
 * listeners; */
#define MAX_BUTTON_PROCESSES	10

/* 
 * The message type to be sent to all registered processes.
 */
#define BUTTONS_MSG_TYPE		1

/* The value that will be posted to the registered processes indicating which
 * button has been pressed. */
#define BUTTON1					1
#define BUTTON2					2

/* The message to be sent to all registered processes. button
 * indicates the button which has been pressed (BUTTON1 or BUTTON2)
 * and type will always be BUTTONS_MSG_TYPE. */

typedef struct buttons_message{
	u8_t button;
	u8_t type;
}buttons_message_t;


/**
 * \brief Configure the IO port of the MSP430.
 * 
 * This function should be called first in order for the others to work. 
 * \hideinitializer
 */
void buttons_init(void);
int buttons_1pressed(void);
int buttons_2pressed(void);
int buttons_register(struct process *proc);

/**
 * The interrupt handler routines.
 */
void button1_interrupt(void);
void button2_interrupt(void);

#endif /*BUTTONS_H_*/
