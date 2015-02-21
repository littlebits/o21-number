/*
*  o21-number.c
*	Authors: littleBits Electronics, Inc.
*
* Copyright 2013 littleBits Electronics
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
*
* This file incorporates work covered by the following copyright and
* permission notice:
*
*	Copyright 2013
*	source: http://www.dsprelated.com/showcode/304.php
*	
*	The code is released under Creative Commons Attribution 3.0. See the license
* 	at <https://creativecommons.org/licenses/by/3.0/us/legalcode> for more details.
*/
 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "lb-led-display.h"

// Specify the system's clock frequency
#define F_CPU 1000000UL

// This calculation is used for alpha in the EMA filter; it determines our cutoff frequency...
#define DSP_EMA_I16_ALPHA(x) ( (uint8_t)(x * 255) )

// output_mode = 0 means we're in NUMBER mode (0-99)
// and output_mode = 1 means we're in VOLTAGE (0.0-5.0) mode
enum output_mode { NUMBER, VOLTAGE }; 
enum output_mode output_mode;

// This is a kind of struct that will be used to store data used in computations and later passed out to the seven-segment LED display...
struct display_data
{
	uint8_t left_digit;
	uint8_t right_digit;
	uint8_t decimal_point;
};

// This variable will designate which of 3 operations we will execute in our while(1) loop:
// CHECK_SWITCH_STATUS, COMPUTE_DISPLAY_DATA, or OUTPUT_DISPLAY_DATA
static volatile uint8_t state = 0;

// This is a flag to be set HIGH in the while(1) state-machine nested IF-Elses, and set LOW in timer2's ISR
static volatile uint8_t operation_complete;


////////////////////////////////// ADC and DSP-related Global Variables //////////////////////////////////

// This variable will take in our ADC readings... it's filled in the ADC ISR
// Note we have a flag (8-bit variable) called "adc_reading_occurred" that will prevent the ISR from re-filling this
// variable until we've successfully iterated through while(1) once with the given sample
static volatile int16_t sample_in_int16;

// This is the averaged output of our LPF
static volatile int16_t sample_out_int16;

// This is the unipolar 8-bit sample between 0 and 256 that we receive after lowpass filtering...
static volatile uint8_t filtered_sample;

// This variable functions as a flag to indicate that it's time to take in another ADC sample. 
// The ADC ISR watches for it, and only takes in a new ADC reading if the flag is set HIGH. Once it takes the 
// new sample in, it sets the flag LOW and waits again...
// Meanwhile in our main while() loop, we run DSP on incoming samples. When finished processing a sample, we set
// the flag HIGH there to indicate that we're ready to grab a new sample from ADC ISR 
static volatile uint8_t ready_for_adc_sample;

/////////////////////////////////////////////////////////////////////////////////////////////////////////


// First-order IIR LPF on incoming ADC signal
static inline int16_t dsp_ema_i16(int16_t in, int16_t average, uint8_t alpha);

// Functions to compute the outgoing display data
static inline void calc_number(struct display_data *display_values);
static inline void calc_voltage(struct display_data *display_values);

// Send the computed data out to the LED SSD
static inline void output_display(struct display_data *display_values);

// Turn off all LEDs on the SSD
static inline void all_leds_off();


int main(void)
{
	// This struct holds data specifying which integers/dec points to display on the LED SSD
	struct display_data new_output;
	
	// Initialize the output pins to proper mode to control the LED segments
	init_led_outputs();
	
	// Initialize the ADC. It will take readings automatically, calling ISR(ADC_vect) to do so.
	// Use a clock frequency of 1/32nd of sysclock (so 31.25 kHz)
	// A given ADC reading takes 13.5 cycles, so the ADC will have a sample rate of 2.314 kHz.
	init_adc(32);							
	
	// Initialize the switch. Be sure to specify which pin/port for this switch in lb-debounce.h
	init_switch();
	
	// Turn on the internal pull-up resistor for the aforementioned switch				
	toggle_switch_pullup(1);				
	
	// Start this timer which will cycle us through the 3 states
	init_timer2();
	
	/*
		Note: If the watchdog is accidentally enabled, for example by a runaway pointer or brown-out
		condition, the device will be reset and the watchdog timer will stay enabled. If the code is not set
		up to handle the watchdog, this might lead to an eternal loop of time-out resets. To avoid this sit-
		uation, the application software should always clear the watchdog system reset flag (WDRF)
		and the WDE control bit in the initialization routine, even if the watchdog is not in use.
	*/
	
	// clear the watchdog system reset flag
	MCUSR &= ~(WDRF << 1);
	
	// Enable the watchdog timer. We specify a max-out period of 60 mS
	wdt_enable(WDTO_60MS);
  
	
    while(1)
    {		
		
		if(state = !ready_for_adc_sample)
		{	
			// DSP happens now:
			// Lowpass filter our incoming ADC samples with a 1st-order exponential moving average IIR lowpass filter
			sample_out_int16 = dsp_ema_i16(sample_in_int16, sample_out_int16, DSP_EMA_I16_ALPHA(0.01));
			
			// Set this flag HIGH again so that the ADC can take in a new sample in its next ISR
			ready_for_adc_sample = 1;
		}
		
		
		// The following 3 operations will each execute one at a time, at a rate of roughly 333 Hz
		// The rate of the execution of these depend on timer2's ISR:
		if(!operation_complete)
		{
			switch(state)
			{	
				// case: CHECK_SWITCH_STATUS
				case 0:
				
					// Read the current mode from the switch:
					if( bit_is_set(PINC, PC5) )
						output_mode = VOLTAGE;
					else
						output_mode = NUMBER;
				
				break;

				// case: COMPUTE_DISPLAY_DATA
				case 1:
				
					/// First pull the most recently filtered sample from our ADC stream:
					// Using addition, offset our reading up to the unipolar range 0 - 65535, then downscale it into the 8-bit output range.
					// The result is an unsigned 8-bit value between 0 and 256 that is appropriate for the Number Bit display calculations:
					filtered_sample = (sample_out_int16 + 0x8000) >> 8;
			
					// Now use  that sample (its a global var) to compute data for the 7-segment LED display:
					if(output_mode == NUMBER)
						calc_number(&new_output);
					else // output_mode == VOLTAGE
						calc_voltage(&new_output);
				
				break;
	
				// case: OUTPUT_DISPLAY_DATA
				case 2:
				
					// Now we'll output numbers to the LED display depending on our current mode:
					output_display(&new_output);
				
				break;
				
			} // end switch(state)
			
			// Set this flag HIGH so that timer2's ISR will know to move us into next state...
			operation_complete = 1;
		
		} // end if(!operation_complete)
		
		// go_to_sleep();					// Be sure to set sleep mode before enabling this
		
		// Kick the dog AKA reset the watchdog timer thereby avoiding an unwanted system reset
		wdt_reset();
				
    } // end while(1)
	
} // end main()


ISR (TIMER2_COMPA_vect)
{
	if(operation_complete)
	{
		state++;	
		state = state%3;
		operation_complete = 0;
	}		
}



/*	
	This interrupt routine will be called every time that the ADC executes a new reading.
	ADC will have a sample rate of 2.314 kHz.
	We're using a 1-pole anti-aliasing LPF where Fc = 16 Hz 
*/
ISR(ADC_vect) 
{	
	// This flag prevents us from re-loading the sample_in variable while we're in the middle of DSP:
	if(ready_for_adc_sample)
	{
		// Recall that our AVR ADC's return 10-bit wide unipolar ADC samples, ranging from 0-1023.
		// First perform a subtraction to move each reading into the range of -512 to 512, then upscale it into the 16-bit range.
		// The result is a signed 16-bit sample ready for DSP.
		sample_in_int16 = (ADC - 0x200) << 6;
		
		// Set the flag low to indicate that we have a fresh reading waiting to be filtered in the while(1) loop
		ready_for_adc_sample = 0;
	}
}	 


// This is my modified version of original source code from: http://www.dsprelated.com/showcode/304.php
// This is a first order IIR lowpass filter, called an Exponential Moving Average filter.
// Decreasing the value of alpha lowers the cutoff frequency of the filter
int16_t dsp_ema_i16(int16_t in, int16_t average, uint8_t alpha)
{
	int32_t tmp0; // calcs must be done in 32-bit math to avoid overflow
	tmp0 = (int32_t)in * (alpha) + (int64_t)average * (256 - alpha);
	return (int16_t)((tmp0 + 128) / 256); // scale back to 32-bit (with rounding)
}



// Output all of the calculated values to the seven segment LED display:
static inline void output_display(struct display_data *display_values)
{
	display_left[display_values->left_digit]();

	display_right[display_values->right_digit]();
	
	display_decimal(display_values->decimal_point);
}


static inline void calc_number(struct display_data *display_values)
{	
	
	if(filtered_sample > 249)
		filtered_sample = 249;
	
	// First let's calculate the 10's digit:
	// At this point the signal now ranges from 0-255.
	// We divide this into 10 sections (each with a difference of 26)
	// So tens_val will range from 0 to 9:
	uint16_t temp_left = filtered_sample/25;
	
	// Put this in here as a safety-guard to be sure we never try to access an address space beyond the limits of arrays
	if(temp_left > 9)
		temp_left = 9;
	
	// And finally pass these values on to our struct that holds data to be displayed:
	display_values->left_digit = temp_left;
	
	// Now to calculate the ones digit:
	// First we remove the largest multiple of 25 from the variable.
	// After this operation, remainder will range from 0 to 24:
	// We would want to divide 24 by 2.4 to get 10 divisions, but instead we shift both those operands up 8 bits:
	// So (24/10) << 8 = 6144/10 = 614... Divide by this value to get results ranging from 0-9... 
	// This is fixed point arithmetic to avoid floating point computations
	
	uint16_t remainder = (filtered_sample%25) << 8;
	uint8_t ones_val = remainder/614;
	
	// Set any excessively high values to 9 (this operation will introduce a slight yet negligible inaccuracy to our display)
	if(ones_val > 9)
		ones_val = 9;
	
	// Pass on the computed ones value
	display_values->right_digit = ones_val;
	
	// Finally, make sure that the decimal point is turned off
	display_values->decimal_point = OFF;
	
}


// Recall for voltage mode, a shift of 5 bits (out of 256) in ADC readings corresponds to a change of 0.1 V (which should cause a display change)
static inline void calc_voltage(struct display_data *display_values)
{
	// Set up this new variable to hold our adc readings for these calculations
	// Apply an offset of two bits just for accuracy...	
	// uint16_t temp_adc = filtered_sample + 1;
	
	uint16_t temp_adc = filtered_sample + 2;
	
	// At this point the signal now ranges from 0-255.
	// First let's calculate the 1's digit in voltage:
	// We divide the adc reading (ranging from 0-255) by 51.
	// After this operation, ones_digit will range from 0 to 5.
	uint8_t ones_digit = temp_adc/51;
	
	// If the value rises above 5, then cap it
	if (ones_digit > 5)
		ones_digit = 5;
	
	// Pass this computed value back out of the function to our struct that holds data to be displayed
	display_values->left_digit = ones_digit;
	
	// Now to calculate the decimal digit:
	// First we remove the largest multiple of 51 from the variable.
	// After this operation, remainder will vary from 0 to 50:
	uint8_t remainder = temp_adc%51;
	
	// This is a small scale-up patch used because our displays were outputting inaccurately high values in the 4V region.
	if( (ones_digit == 4) && (remainder > 4) )
		remainder -= 2;
		
	// This is a small scale-up patch used because our readings were outputting inaccurately low values in the lower half of the 1 - 1.5V region.
	// if( (ones_digit == 1) && ( remainder < 25) )
	//	remainder += 2;
		
	// Now we divide this value by 5. 
	// After this operation, dec_digit will range from 0 to 10
	uint8_t dec_digit = remainder/5;
	
	// Reduce any values of 10 to 9 (this operation will introduce a slight yet negligible inaccuracy to our display)
	if( dec_digit > 9)
		dec_digit = 9;
		
	// Now pass this computed value out of the function
	display_values->right_digit = dec_digit;

	display_values->decimal_point = ON;
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



static inline void all_leds_off()
{
	PORTB &= ~(LEFT_A | LEFT_B | LEFT_E | LEFT_F | LEFT_G);
	PORTC &= ~(RIGHT_A | RIGHT_F | RIGHT_G);
	PORTD &= ~(LEFT_C | LEFT_D | RIGHT_B | RIGHT_C | RIGHT_D | RIGHT_E | DECIMAL_PT);	
}


