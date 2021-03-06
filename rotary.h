/* Author By: Robert Hall
 * Licence: MIT https://choosealicense.com/licenses/mit/
 * Adapted from pong.c which has the following license notice:
 *   Author: Steve Gunn
 *   License: Licensed under the Creative Commons Attribution License.
 */
 
#include <stdint.h>

#define ROTA	PE4
#define ROTB	PE5
#define SWC		PE7
#define SWN		PC2
#define SWE		PC3
#define SWS		PC4
#define SWW		PC5

extern volatile int8_t rotary;

void init_rotary();
int8_t get_rotary();
uint8_t get_switch();

