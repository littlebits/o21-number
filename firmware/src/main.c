/*
*	main.c
*	uC = ATMega168-PA
*	SysClk = 1MHz (originating from internal 8 MHz RC oscillator and 1/8 prescalar)
*	Created: 5/27/2014 11:11:43 AM
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


#include "o21-number.h"
#include "led_display.h"
#include "pwm_lookup.h"


// FUNCTION PROTOTYPES
static inline int16_t dsp_ema_i16(int16_t in, int16_t average, uint8_t alpha);	// First-order IIR LPF on incoming ADC signal
static inline void calc_number(struct display_data *display_values);			// Compute the outgoing display data in VALUES mode
static inline void calc_voltage(struct display_data *display_values);			// Compute the outgoing display data in VOLTS mode
static inline void output_display(struct display_data *display_values);			// Send the computed data out to the LED SSD for VALUES or VOLTS modes
static inline void output_display_counter(uint8_t value);						// Send the computed data out to the LED SSD for UP or DOWN modes

void reset_counter(uint8_t);		// Reset the counter value in UP or DOWN modes to the default values
void initialize_display();			// Initialize the LED SSD in UP or DOWN modes
void setup();						// Initialize WDT, LED SSD, pins, and interrupts


int main(void)
{

	setup();		// Initialize everything

	while(1)
	{
				
		// DSP for incoming ADC samples
		// We're using a 1-pole anti-aliasing LPF where Fc = 16 Hz
		//if( (state = !ready_for_adc_sample) )
		if (!ready_for_adc_sample)
		{
			sample_out_int16 = dsp_ema_i16(sample_in_int16, sample_out_int16, DSP_EMA_I16_ALPHA(0.01));		// LPF with 1st order exponential moving average IIR filter
			ready_for_adc_sample = 1;					// Set this flag HIGH again so that the ADC can take in a new sample in its next ISR
		}
		
		// The following 3 operations will execute one at a time at a rate of 333 Hz (rate depends on timer2's ISR)
		if (!operation_complete)
		{
			switch(state)
			{
				// case: READ MODE SWITCH
				case 0:
				
				if (mode_in <= 300)
				{
					switch_mode = UP;
					update_pwm = 1;
				}
				else if (mode_in > 300 && mode_in <= 600){
					switch_mode = DOWN;
					update_pwm = 1;
				}
				else if (mode_in > 600 && mode_in <= 850)
				{
					switch_mode = VALUES;
				}
				else if(mode_in > 850 && mode_in <= 1023)
				{
					switch_mode = VOLTS;
				}
				
				break;
				
				// case: COMPUTE DATA IN VALUES AND VOLTS MODES
				case 1:
				filtered_sample = (sample_out_int16 + 0x8000) >> 8;			// get filtered sample, offset to 0-65535 range, and downscale to 8-bit range
				if (switch_mode == VALUES)
				{
					calc_number(&new_output);								// compute date for SSD
				}
				else if (switch_mode == VOLTS)
				{
					calc_voltage(&new_output);								// compute data for SSD
				}

				break;
				

				// case: OUTPUT DATA TO DISPLAY
				case 2:
				
				if(switch_mode == VALUES || switch_mode == VOLTS)
				{
					output_display(&new_output);						// output to SSD
				}
				else if (switch_mode == UP || switch_mode == DOWN)
				{
					output_display_counter(counter_value);				// output to SSD
				}
				
				break;
				

			} //end switch
			
			operation_complete = 1;			// Set this flag HIGH so that timer2's ISR will know to move us into next state
			
			
		} //end if op complete
		
		
		// On rising edge of reset bitsnap pin, reset the counter to default values
		reset_state = READ_RESET_BUTTON();
		if(reset_state && !last_reset_state)
		reset_counter(switch_mode);
		last_reset_state = reset_state;
		
		wdt_reset();
		
	} //end while
	
} //end main


void setup()
{
	MCUSR &= ~(WDRF << 1);				// Clear the watchdog system reset flag
	wdt_enable(WDTO_60MS);				// Enable the watchdog timer. We specify a max-out period of 60 mS
	wdt_reset();						// Reset the watchdog timer

	adc_init(8);						// Initialize ADC. Use clock frequency of 1/8 of sysclock (125kHz). ADC reading takes 13 cycles, so ADC sample rate is 9.615kHz.
	_delay_ms(10);

	init_pwm_1();						// initialize PWM output and timer 1
	init_timer2();						// Initialize timer 2 to cycle through 3 states
	
	ENABLE_PCINT1();					// Enable the PCINT1
	INIT_PCINT1();						// Initialize PCINT1 trigger on PC0
	
	INIT_SWITCH();						// Initialize the mode switch pin
	INIT_RESET_BUTTON();				// Initialize the reset bitsnap signal pin
	INIT_INPUT_PULSE();					// Initialize the input bitsnap signal pin
	_delay_ms(1);
	
	init_led_outputs();					// Initialize 7 segment display
	initialize_display();				// Initialize display for counter mode

	wdt_reset();						// Reset WDT
}



ISR(PCINT1_vect)
{
	
	// On rising edge on input bitsnap pin, increment or decrement counter value and update pwm for output bitsnap
	// If there is a pulse on the input bitsnap
	if(READ_INPUT_PULSE())
	{
		switch(switch_mode)
		{
			case UP:
			// if we haven't hit our lower boundary
			if(counter_value < DOWN_DEFAULT)
			{
				counter_value++;	// increment
				update_pwm = 1;		// set flag to update PWM output
			}
			break;
			
			case DOWN:
			// if we haven't hit our upper boundary
			if(counter_value > UP_DEFAULT)
			{
				counter_value--;	// decrement
				update_pwm = 1;		// set flag to update PWM output
			}
			break;
			
			default:
			break;
			
		}
		
	}

	
}

// Interrupt should fire at a rate of ~1kHz
ISR(TIMER1_COMPA_vect)
{
	if (switch_mode == VALUES || switch_mode == VOLTS)
	{
		OUTPUT_BITSNAP = sig_in;
	}
	else if (switch_mode == UP || switch_mode == DOWN)
	{
		if(update_pwm)
		{
			OUTPUT_BITSNAP = pwm_lookup[counter_value];	//set output bitsnap pin to the scaled PWM output
			update_pwm = 0;								// clear the flag to update the PWM output
		}
	}

	
}


// Interrupt should fire at a rate of ~1kHz
ISR (TIMER2_COMPA_vect)
{
	mode_in = adc_read(5);			// read adc from switch mode pin

	if(operation_complete)
	{
		state++;
		state = state%3;
		operation_complete = 0;
	}
}


// This interrupt routine will be called every time that the ADC executes a new reading.
ISR(ADC_vect)
{
	
	if (switch_mode == VALUES || switch_mode == VOLTS)
	{
		// if we're ready to take in another ADC sample
		if(ready_for_adc_sample)
		{
			sig_in = adc_read(0);						// read adc on input bitsnap pin
			sample_in_int16 = (sig_in - 0x200) << 6;	// move reading to range of -512 to 512, upscale into 16-bit range
			ready_for_adc_sample = 0;					// clear flag so DSP can happen
		}
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

// Output counter mode values to LED SSD display:
static inline void output_display_counter(uint8_t value)
{
	uint8_t tens = value / 10;		// determine the most significant digit
	uint8_t ones = value % 10;		// determine the least significant digit
	
	display_left[tens]();			// put the MSD on the display
	display_right[ones]();			// put the LSD on the display
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
	//if( (ones_digit == 1) && ( remainder < 25) )
	//remainder += 2;
	
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


void reset_counter(uint8_t sw_state)
{
	// if the bit is set to UP mode
	if(sw_state == UP)
	counter_value = UP_DEFAULT;
	// else the bit is set to DOWN mode
	else if (sw_state == DOWN)
	counter_value = DOWN_DEFAULT;
	
	update_pwm = 1;		// set the flag to update the PWM output
}

void initialize_display()
{
	mode_in = adc_read(5);		// read adc input from switch
	
	if (mode_in < 300)
	{
		switch_mode = UP;
		reset_counter(switch_mode);
		output_display_counter(counter_value);
	}
	else if (mode_in > 300 && mode_in < 600){
		switch_mode = DOWN;
		reset_counter(switch_mode);
		output_display_counter(counter_value);
	}
	else if(mode_in > 600 && mode_in < 850)
	{
		switch_mode = VALUES;
	}
	else if(mode_in > 850 && mode_in <1023)
	{
		switch_mode = VOLTS;
	}

}
