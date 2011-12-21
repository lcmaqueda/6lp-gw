/**
 * \file		random.c
 *
 * \brief		Architecture-specific functions for pseudo-random number
 *              generation.
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#include "dev/hal_cc2520.h"
#include "contiki.h"

/*---------------------------------------------------------------------------*/
void
random_init(unsigned short seed)
{
  return;
}

/*---------------------------------------------------------------------------*/
u16_t
random_rand(void)
{
  return CC2520_RANDOM16();
}
/*---------------------------------------------------------------------------*/
