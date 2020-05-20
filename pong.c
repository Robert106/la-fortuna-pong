/* Author By: Robert Hall
 * Licence: MIT https://choosealicense.com/licenses/mit/
 * Adapted from pong.c which has the following license notice:
 *   Author: Steve Gunn
 *   License: Licensed under the Creative Commons Attribution License.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"
#include "rotary.h"
#include "led.h"


#define BALL_SIZE	4
#define BAT_WIDTH	50
#define BAT_HEIGHT	2
#define BAT_INC		10


const rectangle start_bat = {(LCDWIDTH-BAT_WIDTH)/2, (LCDWIDTH+BAT_WIDTH)/2, LCDHEIGHT-BAT_HEIGHT-1, LCDHEIGHT-1};
const rectangle start_ball = {(LCDWIDTH-BALL_SIZE)/2, (LCDWIDTH+BALL_SIZE)/2, 0, BALL_SIZE-1};
const rectangle start_top_bat = {(LCDWIDTH-BAT_WIDTH)/2, (LCDWIDTH+BAT_WIDTH)/2, 0, BAT_HEIGHT+1};		// Defines the top bat

int8_t xinc = 1, yinc = 1;
volatile ballColour = RED;
volatile batColour = RED;
int colours[] = {RED, RED, BLUE, BLUE, GREEN, GREEN, YELLOW, YELLOW, MAGENTA, MAGENTA, WHITE, WHITE};
volatile rectangle bat, ball, last_bat, last_ball, top_bat, last_top_bat;
volatile uint16_t score;
volatile uint8_t lives = 3;
volatile uint8_t fps = 0;

ISR(INT6_vect)
{
	fill_rectangle(last_ball, display.background);
	fill_rectangle(ball, ballColour);
	fill_rectangle(last_bat, display.background);
	fill_rectangle(bat, batColour);
	fill_rectangle(last_top_bat, display.background);	// clear old bat position and show new one
	fill_rectangle(top_bat, batColour);
	last_ball = ball;
	last_bat = bat;
	last_top_bat = top_bat;
	char buffer[4];
	sprintf(buffer, "%03d", score);
	display_string_xy(buffer, 200, 20);
	fps++;
}

ISR(TIMER1_COMPA_vect)
{
	ball.left   += xinc;
	ball.right  += xinc;
	ball.top    += yinc;
	ball.bottom += yinc;
	// Move top bat so it follows the ball
	top_bat.left += xinc;
	top_bat.right += xinc;
	if (ball.right >= display.width-1 || ball.left <= 0) {
		xinc = -xinc;
	}if (ball.top <= 0)
		yinc = -yinc;
	if (ball.bottom == display.height-1 && ball.left <= bat.right && ball.right >= bat.left) {
		yinc = -yinc;
		if (!(++score % 10))
			OCR1A >>= 1;
	}
	if (rotary<0 && bat.left >= BAT_INC) {
		bat.left  -= BAT_INC;
		bat.right -= BAT_INC;
	}
	if (rotary>0 && bat.right < display.width-BAT_INC) {
		bat.left  += BAT_INC;
		bat.right += BAT_INC;
	}
	if (~PINC & _BV(SWN) && bat.left >= 5) {					// If left pressed, "jump" to the left
		bat.left -= 5;
		bat.right -= 5;
	}
	if (~PINC & _BV(SWS) && bat.right < display.width-5) {		// If right pressed, "jump" to the left
		bat.left += 5;
		bat.right += 5;
	}
	rotary = 0;
	if (ball.bottom > display.height) {
		lives--;
		led_brightness(0x03 << (lives << 1));
		ball = start_ball;
		bat = start_bat;
	}
}

ISR(TIMER3_COMPA_vect)
{
	char buffer[4];
	sprintf(buffer, "%03d", fps);
	display_string_xy(buffer, 40, 20);
	fps = 0;
}

int main()
{
	/* Clear DIV8 to get 8MHz clock */
	CLKPR = (1 << CLKPCE);
	CLKPR = 0;
	init_rotary();
	init_led();
	init_lcd();
	set_frame_rate_hz(61); /* > 60 Hz  (KPZ 30.01.2015) */
	/* Enable tearing interrupt to get flicker free display */
	EIMSK |= _BV(INT6);
	/* Enable rotary interrupt to respond to input */
	EIMSK |= _BV(INT4) | _BV(INT5);
	/* Enable game timer interrupt (Timer 1 CTC Mode 4) */
	TCCR1A = 0;
	TCCR1B = _BV(WGM12);
	TCCR1B |= _BV(CS10);
	TIMSK1 |= _BV(OCIE1A);
	/* Enable performance counter (Timer 3 CTC Mode 4) */
	TCCR3A = 0;
	TCCR3B = _BV(WGM32);
	TCCR3B |= _BV(CS32);
	TIMSK3 |= _BV(OCIE3A);
	OCR3A = 31250;
	/* Play the game */
	mainMenu();
}

void mainMenu() {
    /* Display Stuff, shows options */
	clear_screen();
	display_string_xy("MAIN MENU", 88, 70);
	display_string_xy("CENTRE: Start Game", 63, 95);
	display_string_xy("LEFT: Colour Customisation", 33, 120);
	display_string_xy("RIGHT: Difficulty", 65, 145);
	/* forever check if button pressed */
	for (;;) {
		if (~PINC & _BV(SWN)) { customColours(); }   // If left pressed, run custom colours
		if (!(PINE & _BV(PE7))) {                    // If centre pressed, clear screen and run pre-made game
			do {
				clear_screen();
				last_bat = bat = start_bat;
				last_ball = ball = start_ball;
				last_top_bat = top_bat = start_top_bat;
				score = 0;
				OCR1A = 65535;
				led_on();
				sei();
				while(lives);
				cli();
				led_off();
				display_string_xy("Game Over", 90, 150);
				PORTB |= _BV(PB6);
				while(PINE & _BV(SWC)) {
					if (PINB & _BV(PB6))
						led_on();
					else
						led_off();
				}
				clear_screen();
			} while(1);
		} if (~PINC & _BV(SWS)) { changeDifficulty(); }	// If right pressed, open difficulty menu
	}
}

void customColours() {
    /* Display stuff */
	clear_screen();
	display_string_xy("COLOUR CUSTOMISATION", 60, 70);
	display_string_xy("LEFT: Change Ball Colour", 56, 95);
	display_string_xy("(CURRENT: RED)", 69, 110);
	display_string_xy("RIGHT: Change Paddle Colour", 52, 130);
	display_string_xy("(CURRENT: RED)", 69, 145);
	display_string_xy("DOWN: Go Back", 70, 165);
	int counter = 0;
	int counterBat = 0;
	/* forever loop to check if button pressed */
	for (;;) {
		if (~PINC & _BV(SWN)) {					// Press left to change ball colour
			/* Iterate list to get the current colour, display to user. If over max, repeat from start */
			counter = counter + 1;
			if (counter == 12) { counter = 1; }
			ballColour = colours[counter];
			clear_screen();
			display_string_xy("COLOUR CUSTOMISATION", 60, 70);
			display_string_xy("LEFT: Change Ball Colour", 46, 95);
			if (counter == 0 || counter == 1) { display_string_xy("(CURRENT: RED)", 69, 110); }
			if (counter == 2 || counter == 3) { display_string_xy("(CURRENT: BLUE)", 69, 110); }
			if (counter == 4 || counter == 5) { display_string_xy("(CURRENT: GREEN)", 69, 110); }
			if (counter == 6 || counter == 7) { display_string_xy("(CURRENT: YELLOW)", 69, 110); }
			if (counter == 8 || counter == 9) { display_string_xy("(CURRENT: MAGENTA)", 69, 110); }
			if (counter == 10 || counter == 11) { display_string_xy("(CURRENT: WHITE)", 69, 110); }
			display_string_xy("RIGHT: Change Paddle Colour", 42, 130);
			if (counterBat == 0 || counterBat == 1) { display_string_xy("(CURRENT: RED)", 69, 145); }
			if (counterBat == 2 || counterBat == 3) { display_string_xy("(CURRENT: BLUE)", 69, 145); }
			if (counterBat == 4 || counterBat == 5) { display_string_xy("(CURRENT: GREEN)", 69, 145); }
			if (counterBat == 6 || counterBat == 7) { display_string_xy("(CURRENT: YELLOW)", 69, 145); }
			if (counterBat == 8 || counterBat == 9) { display_string_xy("(CURRENT: MAGENTA)", 69, 145); }
			if (counterBat == 10 || counterBat == 11) { display_string_xy("(CURRENT: WHITE)", 69, 145); }
			display_string_xy("DOWN: Go Back", 70, 165);
		}
		if (~PINC & _BV(SWS)) {					// Press right to change paddle colour
			/* Iterate list to get the current colour, display to user. If over max, repeat from start */
			counterBat = counterBat + 1;
			if (counterBat == 12) { counterBat = 1; }
			batColour = colours[counterBat];
			clear_screen();
			display_string_xy("COLOUR CUSTOMISATION", 60, 70);
			display_string_xy("LEFT: Change Ball Colour", 46, 95);
			if (counter == 0 || counter == 1) { display_string_xy("(CURRENT: RED)", 69, 110); }
			if (counter == 2 || counter == 3) { display_string_xy("(CURRENT: BLUE)", 69, 110); }
			if (counter == 4 || counter == 5) { display_string_xy("(CURRENT: GREEN)", 69, 110); }
			if (counter == 6 || counter == 7) { display_string_xy("(CURRENT: YELLOW)", 69, 110); }
			if (counter == 8 || counter == 9) { display_string_xy("(CURRENT: MAGENTA)", 69, 110); }
			if (counter == 10 || counter == 11) { display_string_xy("(CURRENT: WHITE)", 69, 110); }
			display_string_xy("RIGHT: Change Paddle Colour", 42, 130);
			if (counterBat == 0 || counterBat == 1) { display_string_xy("(CURRENT: RED)", 69, 145); }
			if (counterBat == 2 || counterBat == 3) { display_string_xy("(CURRENT: BLUE)", 69, 145); }
			if (counterBat == 4 || counterBat == 5) { display_string_xy("(CURRENT: GREEN)", 69, 145); }
			if (counterBat == 6 || counterBat == 7) { display_string_xy("(CURRENT: YELLOW)", 69, 145); }
			if (counterBat == 8 || counterBat == 9) { display_string_xy("(CURRENT: MAGENTA)", 69, 145); }
			if (counterBat == 10 || counterBat == 11) { display_string_xy("(CURRENT: WHITE)", 69, 145); }
			display_string_xy("DOWN: Go Back", 70, 165);
		}
		while (~PINC & _BV(SWW)) { mainMenu(); }	// Press down to go back (load main menu)
	}
}

void changeDifficulty() {
	/* Display difficulty menu - for now only change lives */
	clear_screen();
	display_string_xy("DIFFICULTY", 80, 70);
	display_string_xy("LEFT: Decrease Lives", 55, 95);
	display_string_xy("RIGHT: Increase Lives", 53, 120);
	display_string_xy("CENTRE: Change Speed", 55, 145);
	display_string_xy("(CURRENT: Easy)", 62, 160);
	display_string_xy("DOWN: Go Back", 70, 185);
	for (;;) {
		if (~PINC & _BV(SWS)) {			// If right pressed, increase lives
			lives = lives + 1;
		} if (~PINC & _BV(SWN)) {		//If left pressed, decrease lives to a minimum of 1
			if (lives >= 2) { lives = lives - 1; }
			else { lives = 1; }
		} if (~PINC & _BV(SWW)) { mainMenu(); }	// Press down to go back (load main menu)
		if (!(PINE & _BV(PE7))) {
			if (xinc == 1 || xinc == 2) {		// Change xinc and yinc value to change how quick the ball moves
				clear_screen();
				display_string_xy("DIFFICULTY", 80, 70);
				display_string_xy("LEFT: Decrease Lives", 55, 95);
				display_string_xy("RIGHT: Increase Lives", 53, 120);
				display_string_xy("CENTRE: Change Speed", 55, 145);
				display_string_xy("DOWN: Go Back", 70, 185);
				if (xinc == 2) {
					xinc = 3;
					yinc = 2.5;
					display_string_xy("(CURRENT: Hard)", 62, 160);
				} if (xinc == 1) {
					xinc = 2;
					yinc = 1.5;
					display_string_xy("(CURRENT: Medium)", 62, 160);
				}
			} else {
				xinc = 1;
				yinc = 1;
				clear_screen();
				display_string_xy("DIFFICULTY", 80, 70);
				display_string_xy("LEFT: Decrease Lives", 55, 95);
				display_string_xy("RIGHT: Increase Lives", 62, 120);
				display_string_xy("CENTRE: Change Speed", 55, 145);
				display_string_xy("(CURRENT: Easy)", 58, 160);
				display_string_xy("DOWN: Go Back", 70, 185);
			}
		}
	}
}
