/*
*	o21_number.h
*	uC = ATMega168-PA
*	SysClk = 1MHz (originating from internal 8 MHz RC oscillator and 1/8 prescalar)
*	Created: 5/22/2014 5:17:46 PM
*	Authors: littleBits Electronics, Inc.
*
* Copyright 2014 littleBits Electronics
*
* This file is part of o21-number.
*
* o21-number is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* o21-number is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License at <http://www.gnu.org/licenses/> for more details.
*/


#ifndef O21_NUMBER_H_
#define O21_NUMBER_H_

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define F_CPU 1000000UL										// 1MHz = 8MHz with 1/8 prescaler fuse

#define DSP_EMA_I16_ALPHA(x) ( (uint8_t)(x * 255) )			// This calculation is used for alpha in the EMA filter; it determines our cutoff frequency...

#define INIT_SWITCH()		{ DDRC &= ~(1 << PC5); PORTC |= (1 << PC5); }	// MODE SWITCH - set PC5 as input with pull-up resistor

#define INIT_RESET_BUTTON()	{ DDRC &= ~(1 << PC1); PORTC |= (1 << PC1); }	// RESET BITSNAP - set PC1 as input with pull-up resistor
#define READ_RESET_BUTTON()	( (PINC >> PC1) & 1 )							// READ THE RESET BUTTON

#define INIT_INPUT_PULSE()	( DDRC &= ~(1 << PC0) )							// INPUT BITSNAP - set as input
#define READ_INPUT_PULSE()	( (PINC >> PC0) & 1 )							// READ THE INPUT BITSNAP PULSE in count modes

#define ENABLE_PCINT1()		{ PCICR |= _BV(PCIE1); }						//enable PCINT1
#define INIT_PCINT1()		{ PCMSK1 |= (1 << PCINT8); }					//enable trigger on PC0

#define OUTPUT_BITSNAP		OCR1A											// define output bitsnap pin


static volatile uint8_t counter_value;			// counter value for the 7-segment display
static volatile uint8_t update_pwm;				// flag for when to update the PWM output signal

static volatile int16_t sample_in_int16;		// Takes in ADC reading on input
static volatile int16_t sample_out_int16;		// Averaged output of our LPF
static volatile uint8_t filtered_sample;		// This is the unipolar 8-bit sample between 0 and 256 that we receive after lowpass filtering
static volatile uint8_t ready_for_adc_sample;	// Flag to indicate when to take another ADC sample
												// ADC ISR takes in a new sample and sets flag LOW, waits until DSP is finished, then flag is set HIGH when ready again

static volatile uint8_t state = 0;					// This variable will designate which of 2 operations we will execute in our while(1) loop
static volatile uint8_t operation_complete = 0;		// This is a flag to be set HIGH in the while(1) state-machine nested IF-Elses, and set LOW in timer2's ISR

static volatile uint16_t mode_in;					// adc value from PC5, used to determine switch position
static volatile uint16_t sig_in = 0;				// adc value from PC0, value from input bitsnap

static volatile uint8_t reset_state = 0;					// Most recent reset bitsnap signal state
static volatile uint8_t last_reset_state = 0;				// Last reset bitsnap signal state

// OUTPUT DISPLAY FOR 7-SEGMENT DISPLAY
struct display_data					//struct will store data passed to 7-segment display
{
	uint8_t left_digit;
	uint8_t right_digit;
	uint8_t decimal_point;
};

struct display_data new_output;		// This struct holds data specifying which integers/dec points to display on the LED SSD


//OUTPUT MODES
enum modes {UP, DOWN, VALUES, VOLTS};		// mode UP counts from 0-99, mode DOWN counts from 99-0, mode VALUES displays 0-99 based on input, mode VOLTS displays 0-5.0V based on input
#define UP_DEFAULT			0				// default for UP mode
#define DOWN_DEFAULT		99				// default for DOWN mode

enum modes switch_mode;						// This specifies which mode the switch is in


//FUNCTION PROTOTYPES
void init_pwm_1();
void init_timer2();
void init_timer1();
void adc_init(uint8_t);
uint16_t adc_read(uint8_t);


#endif /* O21-NUMBER_H_ */