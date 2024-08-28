#ifndef HCSR04_LIB_H
#define HCSR04_LIB_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#define F_CPU 8000000UL
#include <util/delay.h>

#define TRIGER_DDR DDRD
#define ECHO_DDR DDRD
#define TRIGER_PORT PORTD
#define ECHO_PULLUP PORTD
#define TRIGER 4
#define ECHO 2

/*************************************************
 *  API functions
 *************************************************/

void ultrasonic_init(void);
void enable_ex_interrupt(void);
uint32_t ultrasonic_triger(void);

#endif
