/**
 * \file		buttons.c
 *
 * \brief		Architecture-specific button functions. Platform Hogaza v1.2
 * 				contains two LEDs in pins 1.4 and 2.6.
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#include "dev/buttons.h"

static struct process *registered_processes[MAX_BUTTON_PROCESSES];

static buttons_message_t msg; 
static u8_t last_index = 0;
static struct timer button1_timer;
static struct timer button2_timer;
/*----------------------------------------------------------------------------*/

/**
 * Initialize the buttons in the board. This functions must be called at boot.
 */ 
/*----------------------------------------------------------------------------*/
void
buttons_init(void)
{
	_disable_interrupts();
	P1SEL &= ~(1 << BUTTON1_PIN);	/* Set pins 1.4 as I/O */	
	P2SEL &= ~(1 << BUTTON2_PIN);  	/* Set pins 2.6 as I/O */
		
	P1REN |= (1 << BUTTON1_PIN);		/* Enable internal resistor in pin 1.4 */
	P2REN |= (1 << BUTTON2_PIN);		/* Enable internal resistor in pin 2.6 */
	
	P1OUT |= (1 << BUTTON1_PIN);		/* Set internal resisistor as pull-upin P1.4 */
	P2OUT |= (1 << BUTTON2_PIN);		/* Set internal resisistor as pull-upin P2.6 */
	
	P1DIR &= ~(1 << BUTTON1_PIN);  	/* Set pin 1.4 as input */ 
	P2DIR &= ~(1 << BUTTON2_PIN);  	/* Set pin 2.6 as input */
	
	P1IE |= (1 << BUTTON1_PIN);        /* P1.4 interrupt enabled */
  	P1IES |= (1 << BUTTON1_PIN);       /* P1.4 Hi/Lo edge */
  	P1IFG &= ~(1 << BUTTON1_PIN);    /* P1.4 IFG cleared */
  	
  	P2IE |= (1 << BUTTON2_PIN);        /* P2.6 interrupt enabled */
  	P2IES |= (1 << BUTTON2_PIN);       /* P2.6 Hi/Lo edge */
  	P2IFG &= ~(1 << BUTTON2_PIN);    /* P2.6 IFG cleared */
  	 
	msg.type = BUTTONS_MSG_TYPE;/* Set the buttons message type */
	button1_timer.start = 0;
	button2_timer.start = 0;
	
	register_port1IntHandler(BUTTON1_PIN, button1_interrupt);
	register_port2IntHandler(BUTTON2_PIN, button2_interrupt);
	
	_enable_interrupts();
}
/*----------------------------------------------------------------------------*/

/**
 *
 * This function returns whether the button1 is pressed or not.
 *
 * \return 0 if the button 1 is not pressed.
 */
/*----------------------------------------------------------------------------*/
int
buttons_1pressed(void)
{
	return (!(BUTTON1_IN & (1 << BUTTON1_PIN)));
}
/*----------------------------------------------------------------------------*/

/**
 *
 * This function returns whether the button2 is pressed or not.
 *
 * \return 0 if the button 2 is not pressed.
 */
/*----------------------------------------------------------------------------*/
int
buttons_2pressed(void)
{
	return (!(BUTTON2_IN & (1 << BUTTON2_PIN)));
}
/*----------------------------------------------------------------------------*/

/**
 * \brief This function registers a process as a button listener. The
 * process will receive a PROCESS_EVENT_MSG in the event of a button being
 * pressed.
 *
 * \param proc A pointer to the process being registered.
 * \return 1 if success, 0 otherwise.
 */
/*----------------------------------------------------------------------------*/
int 
buttons_register(struct process *proc)
{
	if (last_index == MAX_BUTTON_PROCESSES) {
		/* Can't allocate another listener */
		return 0;
	} else {
		registered_processes[last_index] = proc;
		last_index++;
		return 1;
	}
}
/*----------------------------------------------------------------------------*/

/**
 *
 * This function will be invoked whenever button 1 is pressed
 */
/*----------------------------------------------------------------------------*/
void
button1_interrupt(void) {
	int i;
	
	if (button1_timer.start == 0 || timer_expired(&button1_timer)) {
		timer_set(&button1_timer, CLOCK_SECOND>>2);	
	
		msg.button = 1;
		for (i = 0; i < last_index; i++) {
			process_post(registered_processes[i], PROCESS_EVENT_MSG, &msg);
		}
	}
	clear_ifg1();		
}
/*----------------------------------------------------------------------------*/

/**
 *
 * This function will be invoked whenever button 2 is pressed
 */
/*----------------------------------------------------------------------------*/
void button2_interrupt(void) {
	int i;
	
	if (button2_timer.start == 0 || timer_expired(&button2_timer)) {
		timer_set(&button2_timer, CLOCK_SECOND>>2);	
	
		msg.button = 2;
		for (i = 0; i < last_index; i++) {
			process_post(registered_processes[i], PROCESS_EVENT_MSG, &msg);
		}
	}
	clear_ifg2();
}
/*----------------------------------------------------------------------------*/
