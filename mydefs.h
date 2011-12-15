/*
 *	mydefs.h - project-specific types, etc - to be included in every file
 *
 * revision history:
 *
 *	jan 10, 2007 - rolf
 *		restore to 4/24/2006 version.
 *
 *	4/24/2006 - rolf
 *		add button functions.
 *
 *	3/22/2006 - rolf
 *		created.
 *
 */

#define NOP()	__asm__ volatile("nop"::)

typedef unsigned char boolean;		// wasteful?
typedef unsigned char byte;
typedef uint16_t uint16;
typedef uint32_t uint32;

extern void delay_ms(uint8_t ms);
extern void delay_us(byte usec);

//
// check button state.  note: buttons are active low input pins
//
#define button_pressed(pin)		((input_test(pin)==0)?1:0)
