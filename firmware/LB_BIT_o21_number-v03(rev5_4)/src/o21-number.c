/*
*	o21_number.c
*	uC = ATMega168-PA
*	SysClk = 1MHz (originating from internal 8 MHz RC oscillator and 1/8 prescalar)
*	Created: 5/22/2014 5:16:05 PM
*	Authors: littleBits Electronics, Inc.
*			MJ, Rory, Kristin
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

#include "o21-number.h"

//Initialize ADC
void adc_init(uint8_t adc_clk_prescalar)
{
	//The first step is to set the prescalar for the ADC clock frequency:
	
	ADCSRA &= ~( (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) ) ;	// clear the values initially
	
	switch(adc_clk_prescalar)									// then set the proper bits high
	{
		case 2:
		ADCSRA |= (1 << ADPS0);
		break;
		
		case 4:
		ADCSRA |= (1 << ADPS1);
		break;
		
		case 8:
		ADCSRA |= ( (1 << ADPS0) | (1 << ADPS1) );
		break;
		
		case 16:
		ADCSRA |= (1 << ADPS2);
		break;
		
		case 32:
		ADCSRA |= ( (1 << ADPS2) | (1 << ADPS0) );
		break;
		
		case 64:
		ADCSRA |= ( (1 << ADPS2) | (1 << ADPS1) );
		break;
		
		case 128:
		ADCSRA |= ( (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) );
		break;
		
		default:
		ADCSRA |= (1 << ADPS0);
		break;
		
	} // end switch()
	
	
	ADMUX = (1<<REFS0);								// AREF = AVcc
	ADCSRA |= (1 << ADIE); 							// Enable ADC interrupt
	ADCSRA |= (1 << ADEN);  						// Enable the ADC
	sei();											// Enable global interrupts
	ADCSRA |= (1<<ADSC);							// start conversion

}


// Set up Timer2 to generate interrupts
void init_timer2()
{
	// Temporarily disable global interrupts while we're setting up the timer
	cli();

	// Set to Clear Timer on Compare Mode (CTC Mode)
	// Once this timer has counted up to the value stored in OCR2A, it will reset to zero
	TCCR2A |= (1 << WGM21);

	// Drive this timer with:  Sysclk/1024 = 976.56 Hz clock
	TCCR2B |= (1 << CS22)|(1 << CS21)|(1 << CS20);

	// Enable interrupt ISR(TIMER2_COMPA_vect) when timer2 value == OCR2A value
	TIMSK2 |= (1 << OCIE2A);
	
	// Re-enable global interrupts
	sei();

}

void init_pwm_1()
{
	cli();
	
	// Set PB1 as output pin (to send output of PWM i.e. OC1A)
	DDRB |= (1 << PB1);
	
	// Fast PWM Mode where TOP = 0x3FF (i.e. a 1024 value from the 16-bit counter TCNT1)
	// Set output (OC1A) to high when counter (TCNT1) hits TOP (0x3FF)
	// Clear output (OC1A) to low when counter (TCNT1) equals compare (OCR1A)
	TCCR1A = (1 << COM1A1) | (1 << WGM11) | (1 << WGM10);
	
	// Set TCCR1B for Fast PWM Mode, no prescalar so that the entire PWM process runs from our system clock
	// So:  PWM_freq = sysclk/1024 = 976Hz
	TCCR1B = (1 << WGM12)| (1 << CS10);
	
	// Enable interrupt ISR(TIMER1_COMPA_vect) when timer1 value == OCR1A value
	TIMSK1 |= (1 << OCIE1A);
	
	// Set our initial 0% duty cycle with the output comparison register (OCR1A)
	// Our 10-bit count range goes from 0-1023, so we must select OCR1A values within that range:
	OCR1A = 0;

	// Initialize our counter to zero...
	TCNT1 = 0x00;
	
	sei();

}

uint16_t adc_read(uint8_t ch)
{
	ch &= 0b00000111;			// select the corresponding channel 0~7
	ADMUX = (ADMUX & 0xF8)|ch;  // clears the bottom 3 bits before ORing
	
	ADCSRA |= (1<<ADSC);		// start single conversion
	while(ADCSRA & (1<<ADSC));	// wait for conversion to complete
	
	return (ADC);				// return ADC value
}

