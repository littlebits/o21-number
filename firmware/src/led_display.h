/*
 * led_display.h\
 *
 * Created: 5/22/2014 5:18:23 PM
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


#ifndef LED_DISPLAY_H_
#define LED_DISPLAY_H_

#define LEFT_A (1 << PB4)
#define LEFT_B (1 << PB5)
#define LEFT_C (1 << PD6)
#define LEFT_D (1 << PD7)
#define LEFT_E (1 << PB0)
#define LEFT_F (1 << PB3)
#define LEFT_G (1 << PB2)

#define RIGHT_A (1 << PC4)
#define RIGHT_B (1 << PD0)
#define RIGHT_C (1 << PD2)
#define RIGHT_D (1 << PD3)
#define RIGHT_E (1 << PD4)
#define RIGHT_F (1 << PC3)
#define RIGHT_G (1 << PC2)

#define DECIMAL_PT_L (1 << PD5)
#define DECIMAL_PT_R (1 << PD1)


// This enumeration will be used in the display_decimal() function below:
enum decimal_point {OFF, ON};

// This function will control the pin that drives the decimal point of the LED display
inline void display_decimal(int decimal_status)
{
	if(decimal_status == ON)
	PORTD |= DECIMAL_PT_L;			// Turn the LED display for decimal point on
	else 						// else assume decimal_status == OFF, so:
	PORTD &= ~DECIMAL_PT_L;			// Turn the LED display for decimal point off
}

// This function specifies the behavior of the pins that drive the LED display.
// Setting a '1' in these ports designates the pins as outputs (as opposed to inputs or something else)

inline void init_led_outputs()
{
	DDRB |= LEFT_A|LEFT_B|LEFT_E|LEFT_F|LEFT_G;
	DDRC |= RIGHT_A|RIGHT_F|RIGHT_G;
	DDRD |= LEFT_C|LEFT_D|RIGHT_B|RIGHT_C|RIGHT_D|RIGHT_E|DECIMAL_PT_L|DECIMAL_PT_R;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////// The following functions control the pins that drive the LED display's left digit ////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void l_zero()
{
	PORTB |= LEFT_A|LEFT_B|LEFT_E|LEFT_F;
	PORTB &= ~LEFT_G;
	
	PORTD |= LEFT_C|LEFT_D;
}

void l_one()
{
	PORTB |= LEFT_B;
	PORTB &= ~(LEFT_A|LEFT_E|LEFT_F|LEFT_G);

	PORTD |= LEFT_C;
	PORTD &= ~LEFT_D;
}

void l_two()
{
	PORTB |= LEFT_A|LEFT_B|LEFT_E|LEFT_G;
	PORTB &= ~LEFT_F;

	PORTD |= LEFT_D;
	PORTD &= ~LEFT_C;
}

void l_three()
{
	PORTB |= LEFT_A|LEFT_B|LEFT_G;
	PORTB &= ~(LEFT_F|LEFT_E);

	PORTD |= LEFT_C|LEFT_D;
}

void l_four()
{
	PORTB |= LEFT_F|LEFT_B|LEFT_G;
	PORTB &= ~(LEFT_A|LEFT_E);

	PORTD |= LEFT_C;
	PORTD &= ~LEFT_D;
}

void l_five()
{
	PORTB |= LEFT_A|LEFT_F|LEFT_G;
	PORTB &= ~(LEFT_B|LEFT_E);

	PORTD |= LEFT_C|LEFT_D;
}

void l_six()
{
	PORTB |= LEFT_A|LEFT_E|LEFT_F|LEFT_G;
	PORTB &= ~LEFT_B;

	PORTD |= LEFT_C|LEFT_D;
}

void l_seven()
{
	PORTB |= LEFT_A|LEFT_B;
	PORTB &= ~(LEFT_E|LEFT_F|LEFT_G);

	PORTD |= LEFT_C;
	PORTD &= ~LEFT_D;
}

void l_eight()
{
	PORTB |= LEFT_A|LEFT_B|LEFT_E|LEFT_F|LEFT_G;

	PORTD |= LEFT_C|LEFT_D;
}

void l_nine()
{
	PORTB |= LEFT_A|LEFT_B|LEFT_E|LEFT_F|LEFT_G;
	PORTB &= ~LEFT_E;

	PORTD |= LEFT_C|LEFT_D;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////// The following functions control the pins that drive the LED display's right digit ////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void r_zero()
{
	PORTC |= RIGHT_A|RIGHT_F;
	PORTC &= ~RIGHT_G;

	PORTD |= RIGHT_B|RIGHT_C|RIGHT_D|RIGHT_E;
}

void r_one()
{
	PORTC &= ~(RIGHT_A|RIGHT_F|RIGHT_G);

	PORTD |= RIGHT_B|RIGHT_C;
	PORTD &= ~(RIGHT_D|RIGHT_E);
}

void r_two()
{
	PORTC |= RIGHT_A|RIGHT_G;
	PORTC &= ~RIGHT_F;

	PORTD |= RIGHT_B|RIGHT_D|RIGHT_E;
	PORTD &= ~RIGHT_C;
}

void r_three()
{
	PORTC |= RIGHT_A|RIGHT_G;
	PORTC &= ~RIGHT_F;

	PORTD |= RIGHT_B|RIGHT_D|RIGHT_C;
	PORTD &= ~RIGHT_E;
}

void r_four()
{
	PORTC |= RIGHT_G|RIGHT_F;
	PORTC &= ~RIGHT_A;

	PORTD |= RIGHT_B|RIGHT_C;
	PORTD &= ~(RIGHT_E|RIGHT_D);
}

void r_five()
{
	PORTC |= RIGHT_A|RIGHT_F|RIGHT_G;

	PORTD |= RIGHT_D|RIGHT_C;
	PORTD &= ~(RIGHT_B|RIGHT_E);
}

void r_six()
{
	PORTC |= RIGHT_A|RIGHT_F|RIGHT_G;

	PORTD |= RIGHT_C|RIGHT_D|RIGHT_E;
	PORTD &= ~RIGHT_B;
}

void r_seven()
{
	PORTC &= ~(RIGHT_F|RIGHT_G);
	PORTC |= RIGHT_A;

	PORTD |= RIGHT_C|RIGHT_B;
	PORTD &= ~(RIGHT_D|RIGHT_E);
}

void r_eight()
{
	PORTC |= RIGHT_A|RIGHT_F|RIGHT_G;

	PORTD |= RIGHT_B|RIGHT_C|RIGHT_D|RIGHT_E;
}

void r_nine()
{
	PORTC |= RIGHT_A|RIGHT_F|RIGHT_G;

	PORTD |= RIGHT_B|RIGHT_C|RIGHT_D;
	PORTD &= ~RIGHT_E;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///	These are arrays of pointers to functions. These arrays function like lookup tables for the left and			///
/// 	right display numbers. The functions that we point to control the GPIO pins that drive the LED displays		///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void (*display_left[10])(void) = {l_zero, l_one, l_two, l_three, l_four, l_five, l_six, l_seven, l_eight, l_nine};
void (*display_right[10])(void) = {r_zero, r_one, r_two, r_three, r_four, r_five, r_six, r_seven, r_eight, r_nine};





#endif /* LED_DISPLAY_H_ */