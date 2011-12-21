#ifndef RADIO_DRIVER_H_
#define RADIO_DRIVER_H_

#include "sys/process.h"

/* Driver state */
typedef enum {
	ON, OFF
} radio_driver_state_t;

extern const struct radio_driver radio_driver;

PROCESS_NAME(radio_driver_process);

#endif /*RADIO_DRIVER_H_*/
