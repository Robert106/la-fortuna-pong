/* Author By: Robert Hall
 * Licence: This work is licensed under the Creative Commons Attribution License.
 *           View this license at http://creativecommons.org/about/licenses/
 * Adapted from pong.c which has the following license notice:
 *   Author: Steve Gunn
 *   License: Licensed under the Creative Commons Attribution License.
 */
 
#include <stdint.h>

#define LED		PB7

void init_led();
void led_on();
void led_off();
void led_brightness(uint8_t i);

