#ifndef ADC_H_
#define ADC_H_

#include "contiki.h"

#define TEMP		0xD
#define LIGHT		0xE
#define	HUMIDITY	0xF

typedef struct{
	u16_t light;
	u16_t temp;
	u16_t humidity;
	u16_t battery;
} sensorData_t;

void adc_init(void);
sensorData_t* get_sensor_data(void);


#endif /*ADC_H_*/
