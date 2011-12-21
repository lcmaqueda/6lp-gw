/**
 * \file		clock_arch.h
 *
 * \brief		Architecture-specific clock functions.
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#ifndef __CLOCK_ARCH_H__
#define __CLOCK_ARCH_H__

#include <msp430f5435a.h>
#include "contiki.h"

/**
 * TimerA interval. Defines the TimerA interrupt period.
 */
#define INTERVAL	256

/**
 * A second, measured in system clock time. As the TimerA module is
 * sourced from ACLK (32768Hz) with a divider of 8 and the TimerA interrupt will
 * fire every 256 cycles, we have 32768/8/256 = 16, which is our CLOCK_SECOND
 * value.
 */
#define CLOCK_CONF_SECOND 16


#endif /* __CLOCK_ARCH_H__ */

