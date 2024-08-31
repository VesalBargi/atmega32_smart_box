#include <avr/io.h>
#define F_CPU 8000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/DS3232_lib.h"
#include "lib/i2c_lib.h"
#include "lib/liquid_crystal_i2c_lib.h"
#include "lib/Serial_lib.h"
#include "lib/HCSR04_lib.h"

#define SBIT(r, b) (r |= (1 << b))
#define CBIT(r, b) (r &= ~(1 << b))
#define GBIT(r, b) (r & (1 << b))
#define TOGGLE(r, b) (r ^= (1 << b))

#define ADC_PIN 0
#define ADC_DDR DDRA
#define ADC_PORT PORTA

#define BUTTON_PIN 2
#define BUTTON_DDR DDRB
#define BUTTON_PORT PORTB

#define STEPPER_DDR DDRC
#define STEPPER_PORT PORTC

#define ALARM_PIN 3
#define ALARM_DDR DDRD
#define ALARM_PORT PORTD

#define BUZZER_PIN 7
#define BUZZER_DDR DDRD
#define BUZZER_PORT PORTD

#define CW 1
#define CCW 0

const char *cmd_set_time = "set time";
const char *cmd_help = "help";

char cmd_key[20];
char cmd_first_value[20];
char cmd_second_value[20];
char command[100];

volatile bool half_second = false;
volatile bool one_second = false;
volatile bool two_second = false;
volatile bool three_second = false;
volatile bool ten_second = false;
volatile bool set_time = false;
volatile bool help = false;
volatile bool buzzer = false;
volatile bool box = false;
volatile bool is_open = false;
volatile bool is_press = false;
volatile bool manual = false;

uint8_t counter_box = 0;
uint8_t counter_time = 0;
uint16_t counter_buzzer = 0;
uint8_t counter_ultrasonic = 0;
uint32_t distance;
uint8_t temp_integer;
float temp_decimal;

LiquidCrystalDevice_t lcd;
DateTime_t t;

void int1_init(void)
{
	// alarm -> pull up
	SBIT(ALARM_PORT, ALARM_PIN);
	// INT1 -> falling edge
	MCUCR |= (1 << ISC11);
	MCUCR &= ~(1 << ISC10);
	// INT1 -> interrupt enable
	GICR |= (1 << INT1);
}

void int2_init(void)
{
	// button -> pull up
	SBIT(BUTTON_PORT, BUTTON_PIN);
	// INT2 -> falling edge
	MCUCSR &= ~(1 << ISC2);
	// INT2 -> interrupt enable
	GICR |= (1 << INT2);
}

void timer0_init(void)
{
	// timer 0 -> interrupt enable
	TIMSK |= (1 << TOIE0);
	// prescaler -> 1024
	TCCR0 = (1 << CS02) | (1 << CS00);
	// 255 - 78 = 177 -> 10 ms
	TCNT0 = 177;
}

void timer1_init(void)
{
	// timer 1 -> interrupt enable
	TIMSK |= (1 << TOIE1);
	// prescaler -> 1024
	TCCR1B = (1 << CS12) | (1 << CS10);
	// 65535 - 7812 = 57723 -> 1 sec
	TCNT1 = 57723;
}

void stepper_init(void)
{
	// stepper -> output
	SBIT(STEPPER_DDR, 3);
	SBIT(STEPPER_DDR, 4);
	SBIT(STEPPER_DDR, 5);
	SBIT(STEPPER_DDR, 6);
}

void adc_init(void)
{
	// ADC -> input
	CBIT(ADC_DDR, ADC_PIN);
	// ADC -> 2.56V reference voltage
	ADMUX |= (1 << REFS1) | (1 << REFS0);
	// ADC -> ADC enable / interrupt enable / prescaler -> 128
	ADCSRA |= (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void rtc_init(void)
{
	// I2C -> init (400 kHz)
	i2c_master_init(I2C_SCL_FREQUENCY_400);

	// date time -> init (23:59:55 2024/12/31)
	t.Second = 55;
	t.Minute = 59;
	t.Hour = 23;
	t.Day = Sunday;
	t.Date = 31;
	t.Month = 12;
	t.Year = 2024;
	RTC_Set(t);
}

void lcd_init(void)
{
	// LCD -> init (A0 -> H, A1 -> H, A2 -> L)
	lcd = lq_init((0b01000110 >> 1), 16, 2, LCD_5x8DOTS);

	t = RTC_Get();
	if (RTC_Status() == RTC_OK)
	{
		// LCD -> print time
		lq_setCursor(&lcd, 0, 0);
		char time_arr[10];
		sprintf(time_arr, "%02d:%02d:%02d", t.Hour, t.Minute, t.Second);
		lq_print(&lcd, time_arr);

		// LCD -> print date
		lq_setCursor(&lcd, 1, 0);
		char date_arr[10];
		sprintf(date_arr, "%02d/%02d/%02d", t.Year, t.Month, t.Date);
		lq_print(&lcd, date_arr);
	}
}

void alarm_init(void)
{
	// buzzer -> output
	SBIT(BUZZER_DDR, BUZZER_PIN);
	// alarm -> 00:00:00
	RTC_AlarmSet(Alarm1_Match_Hours, 0, 0, 0, 0);
	// alarm -> interrupt enable
	RTC_AlarmInterrupt(Alarm_1, 1);
}

void init(void)
{
	int1_init();
	int2_init();
	timer0_init();
	timer1_init();
	stepper_init();
	adc_init();
	rtc_init();
	lcd_init();
	alarm_init();
	serial_init();
	ultrasonic_init();
}

void split_string_by_space(void)
{
	char first_word[20], second_word[20], third_word[20], fourth_word[20];

	// clear all arrays
	strcpy(first_word, "");
	strcpy(second_word, "");
	strcpy(third_word, "");
	strcpy(fourth_word, "");
	strcpy(cmd_key, "");
	strcpy(cmd_first_value, "");
	strcpy(cmd_second_value, "");

	// store each word of command in its variable
	sscanf(command, "%s %s %s %s", first_word, second_word, third_word, fourth_word);

	// combine first two words of command and store as key
	sprintf(cmd_key, "%s %s", first_word, second_word);
	// store third word of command as first value
	sprintf(cmd_first_value, "%s", third_word);
	// store fourth word of command as second value
	sprintf(cmd_second_value, "%s", fourth_word);
}

void rotate_stepper(int steps, int direction)
{
	// steps -> shifed 3 units to the left
	int step_sequence[4] = {0x20, 0x10, 0x40, 0x08};
	for (int i = 0; i < steps; i++)
	{
		// stepper motor -> clockwise rotation
		if (direction == CW)
		{
			for (int step = 0; step < 4; step++)
			{
				STEPPER_PORT = step_sequence[step];
				_delay_ms(50);
			}
		}

		// stepper motor -> counter clockwise rotation
		if (direction == CCW)
		{
			for (int step = 3; step >= 0; step--)
			{
				STEPPER_PORT = step_sequence[step];
				_delay_ms(50);
			}
		}
	}
}

void adc_read(void)
{
	// ADC -> start conversion
	ADCSRA |= (1 << ADSC);
}

void lcd_render(void)
{
	t = RTC_Get();
	if (RTC_Status() == RTC_OK)
	{
		// LCD -> print time
		lq_setCursor(&lcd, 0, 0);
		char time_arr[20];
		sprintf(time_arr, "%02d:%02d:%02d", t.Hour, t.Minute, t.Second);
		lq_print(&lcd, time_arr);

		// LCD -> print date
		lq_setCursor(&lcd, 1, 0);
		char date_arr[20];
		sprintf(date_arr, "%02d/%02d/%02d", t.Year, t.Month, t.Date);
		lq_print(&lcd, date_arr);
	}

	// LCD -> print temperature
	char temp_arr[10];
	sprintf(temp_arr, "%2d\xDF" "C", temp_integer);
	lq_setCursor(&lcd, 1, 12);
	lq_print(&lcd, temp_arr);

	// LCD -> print distance
	char distance_arr[10];
	sprintf(distance_arr, "%3lu" "cm", distance);
	lq_setCursor(&lcd, 0, 11);
	lq_print(&lcd, distance_arr);
}

void distance_check(void)
{
	// SR04 -> get distance
	distance = ultrasonic_triger();

	// open box lid if distance is 20 cm or less
	if (distance <= 20 && distance > 0)
	{
		box = true;
	}
}

void rtc_set_time(char *time, char *date)
{
	int hour, minute, second;
	int day, month, year;

	// store date and time in its variables
	sscanf(time, "%d:%d:%d", &hour, &minute, &second);
	sscanf(date, "%d/%d/%d", &month, &day, &year);

	// create date and time struct
	t.Second = second;
	t.Minute = minute;
	t.Hour = hour;
	t.Day = Sunday;
	t.Date = day;
	t.Month = month;
	t.Year = year + 2000;
	RTC_Set(t);
}

void command_set_time(void)
{
	// set input date and time and print on lcd
	rtc_set_time(cmd_first_value, cmd_second_value);
	lcd_render();
	serial_send_string("\rDone!\r");
}

void command_help(void)
{
	// print help menu
	serial_send_string("\r******  << Help >>  ******\r");
	serial_send_string("\r   set time XX:YY:ZZ M/D/Y\r");
	serial_send_string("-> Sets the desired time\r");
}

void open_box(void)
{
	// open box lid if it's not already
	if (!is_open)
	{
		rotate_stepper(4, CW);
		is_open = true;
	}
}

void close_box(void)
{
	// close box lid if it's not already
	if (is_open)
	{
		rotate_stepper(4, CCW);
		is_open = false;
	}
}

void button_pressed(void)
{
	// open box lid - manual mode -> ON
	if (!is_open)
	{
		manual = true;
		open_box();
	}
	// close box lid - manual mode -> OFF
	else
	{
		close_box();
		manual = false;
	}
}

void stop_alarm(void)
{
	// alarm -> interrupt disable
	RTC_AlarmCheck(Alarm_1);
	// buzzer -> OFF
	CBIT(BUZZER_PORT, BUZZER_PIN);
}

void get_cmd(char c)
{
	static uint8_t index = 0;

	if (c == '\r')
	{
		command[index] = '\0';
	}
	else
	{
		command[index] = c;
	}

	if (command[index] == '\0')
	{
		split_string_by_space();

		if (strcmp(cmd_key, cmd_set_time) == 0)
		{
			set_time = true;
		}
		else if (strcmp(command, cmd_help) == 0)
		{
			help = true;
		}
		else
		{
			serial_send_string("\rInvalid input!\r");
		}
		strcpy(command, "");
		index = 0;
	}
	else
	{
		index++;
	}
}

int main(void)
{
	init();

	serial_send_string("Hello there!\r");
	serial_send_string("Write help for list of commands.\r");

	sei();

	while (1)
	{
		if (half_second)
		{
			if (!manual)
			{
				distance_check();
			}
			counter_ultrasonic = 0;
			half_second = false;
		}

		if (one_second)
		{
			adc_read();
			lcd_render();
			counter_time = 0;
			one_second = false;
		}

		if (three_second)
		{
			stop_alarm();
			counter_buzzer = 0;
			three_second = false;
		}

		if (ten_second)
		{
			if (!manual)
			{
				close_box();
			}
			counter_box = 0;
			ten_second = false;
		}

		if (box)
		{
			if (!manual)
			{
				open_box();
			}
			counter_box = 0;
			box = false;
		}

		if (buzzer)
		{
			SBIT(BUZZER_PORT, BUZZER_PIN);
			counter_buzzer = 0;
			buzzer = false;
		}

		if (is_press)
		{
			button_pressed();
			is_press = false;
		}

		if (set_time)
		{
			command_set_time();
			set_time = false;
		}

		if (help)
		{
			command_help();
			help = false;
		}
	}

	return 0;
}

ISR(INT1_vect)
{
	buzzer = true;
}

ISR(INT2_vect)
{
	is_press = true;
}

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 177;
	TIFR = (1 << TOV0);

	counter_ultrasonic++;
	counter_time++;

	if (counter_ultrasonic == 50)
	{
		half_second = true;
	}

	if (counter_time == 100)
	{
		one_second = true;
	}
}

ISR(TIMER1_OVF_vect)
{
	TCNT1 = 57723;
	TIFR = (1 << TOV1);

	counter_buzzer++;
	counter_box++;

	if (counter_buzzer == 3)
	{
		three_second = true;
	}

	if (counter_box == 10)
	{
		ten_second = true;
	}
}

ISR(ADC_vect)
{
	uint16_t temp = ADC;
	temp_decimal = (float)temp / 10 - 0.2;
	temp_integer = (uint8_t)temp_decimal;
}

#if SERIAL_INTERRUPT == 1
ISR(USART_RXC_vect)
{
	char c = UDR;
	UDR = c;

	get_cmd(c);
}
#endif
