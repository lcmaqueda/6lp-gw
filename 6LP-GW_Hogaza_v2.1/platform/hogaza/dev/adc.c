#include <msp430f5435a.h>
#include "adc.h"

sensorData_t data;

void adc_init(void){
	P7SEL |= 0xE0;
	ADC12CTL0 |= ADC12ON + ADC12SHT1_4;
	ADC12CTL1 |= ADC12SHP;
}

sensorData_t* get_sensor_data(void){
	ADC12CTL0 |= ADC12MSC + ADC12REFON;
	ADC12CTL1 |= ADC12CONSEQ_1;
	ADC12MCTL11 |=  ADC12INCH_11 + ADC12SREF_1;  //(AVCC – AVSS) / 2
	ADC12MCTL13 |= ADC12INCH_13;                 
	ADC12MCTL14 |= ADC12INCH_14;                 
	ADC12MCTL15 |= ADC12INCH_15 + ADC12EOS;
	ADC12IE |= ADC12IE15;
	ADC12CTL0 |= ADC12ENC;                    
	ADC12CTL0 |= ADC12SC;
	return &data;
}
	
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
  
  switch(__even_in_range(ADC12IV,36))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6: break;                           // Vector  6:  ADC12IFG0
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;    	                    // Vector 32:  ADC12IFG13
  case 34: break;     						// Vector 34:  ADC12IFG14
  case 36:
  	data.battery = ADC12MEM11;
  	data.temp = ADC12MEM13; 
  	data.light = ADC12MEM14;
  	data.humidity = ADC12MEM15;
  	break;									// Vector 36:  ADC12IFG15
  default: break; 
  }
}
