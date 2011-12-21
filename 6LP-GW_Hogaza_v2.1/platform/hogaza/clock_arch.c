/**
 * \file		clock_arch.c
 *
 * \brief		Architecture-specific clock function.
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#include "clock_arch.h"
/**
 * Static variables 
 */ 
/*---------------------------------------------------------------------------*/
static volatile clock_time_t ticks;
static volatile u32_t seconds = 0;
/* last_tar is used for calculating clock_fine */
static u16_t last_tar = 0;
/*---------------------------------------------------------------------------*/

/**
 * TimerA0 interrupt handler 
 */ 
/*---------------------------------------------------------------------------*/
void timer_interrupt(void);

#pragma vector = TIMER0_A0_VECTOR
interrupt void
timer_interrupt(void)
{
	++ticks;
    if(0 == (ticks % CLOCK_CONF_SECOND)){
		++seconds;
	}
    last_tar = TA0R;
    /* If there are Event timers pending, notify the event timer module */
    if(etimer_pending()){
        etimer_request_poll();
    }
}
/*---------------------------------------------------------------------------*/

/**
 * Initialize the clock library.
 *
 * This function initializes the clock library and should be called
 * from the main() function of the system.
 */
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{

	/* disable interrupts */
	_disable_interrupts();
	/* Clear timer */
	TA0CTL = TACLR;

	/* And stop it */
	TA0CCR0 = 0;

	/* TA0CCR0 interrupt enabled, interrupt occurs when timer equals TACCR0. */
	TA0CCTL0 = CCIE;

	/* Interrupt after X ms. */
	TA0CCR0 = INTERVAL - 1;

	/* Select ACLK 32768Hz clock, divide by 8. TimerA in Up Mode */
	TA0CTL |= TASSEL_1 | ID_3 | MC_1;

	ticks = 0;

	/* Enable interrupts. */
	_enable_interrupts();
}
/*---------------------------------------------------------------------------*/

/**
 * Get the current clock time.
 *
 * This function returns the current system clock time, measured in system
 * ticks.
 */
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
    return ticks;
}
/*---------------------------------------------------------------------------*/

/**
 * Delays the CPU for approximately i cycles. Note that this function is
 * highly inaccurate and must therefore be used carefully.
 */
/*---------------------------------------------------------------------------*/
void
clock_delay(unsigned int i)
{
    while(i--){
        _nop();
    }
}
/*---------------------------------------------------------------------------*/

/**
 * Get the current clock time measured in seconds.
 *
 * This function returns the current system clock time.
 *
 * \return The current clock time, measured in seconds.
 */
/*---------------------------------------------------------------------------*/
u32_t
clock_seconds(void)
{
    return seconds;
}
/*---------------------------------------------------------------------------*/

/**
 * Returns a tick measured in cpu cycles.
 */
/*---------------------------------------------------------------------------*/
int
clock_fine_max(void)
{
    return INTERVAL;
}
/*---------------------------------------------------------------------------*/

/**
 * Returns the time in clock cycles
 */
/*---------------------------------------------------------------------------*/
unsigned short
clock_fine(void)
{
    u16_t t;

    /* Assign last_tar to a local variable so that it cannot be changed by the
     * interrupt ISR */
    t = last_tar;
    /* perform calc based on t, TAR will not be changed during interrupt */
    return (u16_t) (TA0R - t);
}
/*---------------------------------------------------------------------------*/
