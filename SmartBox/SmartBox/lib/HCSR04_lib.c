#include "HCSR04_lib.h"

uint8_t sensor_working = 0;
uint8_t rising_edge = 0;
uint32_t timer_counter = 0;
uint32_t sensor_distance;

void timer2_init(void)
{
  TIMSK |= (1 << TOIE2);
  TCCR2 = (1 << CS21);
  TCNT2 = 0;
}

void ultrasonic_init(void)
{
  TRIGER_DDR |= (1 << TRIGER);
  ECHO_DDR &= ~(1 << ECHO);
  ECHO_PULLUP |= (1 << ECHO);
  enable_ex_interrupt();
  timer2_init();
  return;
}

void enable_ex_interrupt(void)
{
  MCUCR |= (1 << ISC00);
  GICR |= (1 << INT0);
  return;
}

uint32_t ultrasonic_triger(void)
{
  if (!sensor_working)
  {
    TRIGER_PORT |= (1 << TRIGER);
    _delay_us(15);
    TRIGER_PORT &= ~(1 << TRIGER);
    sensor_working = 1;
  }
  return sensor_distance;
}

ISR(INT0_vect)
{
  if (sensor_working == 1)
  {
    if (rising_edge == 0)
    {
      TCNT2 = 0;
      rising_edge = 1;
      timer_counter = 0;
    }
    else
    {
      sensor_distance = ((timer_counter * 256 + TCNT2) / 58);
      _delay_ms(40);
      timer_counter = 0;
      rising_edge = 0;
    }
  }
}

ISR(TIMER2_OVF_vect)
{
  timer_counter++;
  if (timer_counter > 730)
  {
	  TCNT2 = 0;
	  sensor_working = 0;
	  rising_edge = 0;
	  timer_counter = 0;
  }
}
