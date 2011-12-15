/*
 *	iodefs.h - project-specific I/O pin definitions and some macros/functions to go with them
 *
 *		the inline functions, output_high() and output_low(), allow a simple syntax for
 *		setting/clearing I/O pins without having to remember which port (e.g. PORTA, PORTB, etc)
 *		the pin is assigned to.
 *		similarly, the inline function input_test() allows reading a pin.
 *
 *		for example, to set the SPKR I/O pin high (PB1), we do:
 *			output_high(SPKR);
 *
 *		because these are inline functions, they generate efficient code.  (1 instruction)
 *
 *		note: each I/O pin still needs to be configured as input or output.
 *		this is typically done in the avrinit() function.
 *
 *
 *	author: rolf van widenfelt (rolfvw@pizzicato.com)
 *
 *
 *	specific hardware setup:
 *
 *	- Mignonette v1.0 hardware (atmega88)
 *
 *	revision history:
 *
 *	- apr 18, 2008 - rolf
 *		minor fix to input_test() to eliminate compiler warning.
 *		(control reaches end of non-void function)
 *
 *	- mar 18, 2008 - rolf
 *		rework for mignonette v0.1 pin definitions.
 *
 *	- 3/23/2007 - rolf
 *		created.
 *
 */

//
// note: simply add 0,8,16 or 24 for PORTA,B,C or D respectively, to the pin define for this AVR pin

//#define _PA		0	
/* no PORTA on atmega8/48/88/168 */

#define _PB		8
#define _PC		16
#define _PD		24


//
// project-specific pin definitions go here.
//
// note: TxD (PD1) is used for the UART (as an output), so doesn't need to be declared here
//
// note: changes here MUST be updated in avrinit() too!  (see PORTn and DDRn settings)
//

#define SW1			_PB+PB0		/* input w/ pullup */
#define SPKR		_PB+PB1
#define SW2			_PB+PB2		/* input w/ pullup */
#define RC1			_PB+PB3
#define RC2			_PB+PB4
#define RC3			_PB+PB5
#define RC4			_PB+PB6
#define RC5			_PB+PB7

#define SW3			_PC+PC0		/* input w/ pullup */
#define GC1			_PC+PC1
#define GC2			_PC+PC2
#define GC3			_PC+PC3
#define GC4			_PC+PC4
#define GC5			_PC+PC5

#define ROW1		_PD+PD0
#define ROW2		_PD+PD1
#define ROW3		_PD+PD2
#define ROW4		_PD+PD3
#define ROW5		_PD+PD4
#define ROW6		_PD+PD5
#define ROW7		_PD+PD6
#define SW4			_PD+PD7		/* input w/ pullup */


//
// end of project-specific pin definitions.
//


//
// private macros!
// this is the recommended way to do a bit set/clear operation in avr-gcc
// but this macro should only be used by the inline functions below.
//

#define _output_high(port,bit)	port |= _BV(bit)
#define _output_low(port,bit)	port &= ~_BV(bit)


#define _input_test(port,bit)	(port & _BV(bit))

//
// these inline functions will generate efficient code for I/O port bit set and clear operations.
//
// this determines the actual port by examining the pin number (e.g. PORTA is 0 - 7, PORTB is 8-15, etc)
//

static inline void output_high(unsigned char pin)
{
#ifdef NOTDEF
	if (pin < 8) {
		_output_high(PORTA,pin);
	} else
#endif
	if (pin >= 8 && pin < 16) {
		_output_high(PORTB,(pin-8));
	} else if (pin >= 16 && pin < 24) {
		_output_high(PORTC,(pin-16));
	} else if (pin >= 24 && pin < 32) {
		_output_high(PORTD,(pin-24));
	}
}

static inline void output_low(unsigned char pin)
{
#ifdef NOTDEF
	if (pin < 8) {
		_output_low(PORTA,pin);
	} else
#endif
	if (pin >= 8 && pin < 16) {
		_output_low(PORTB,(pin-8));
	} else if (pin >= 16 && pin < 24) {
		_output_low(PORTC,(pin-16));
	} else if (pin >= 24 && pin < 32) {
		_output_low(PORTD,(pin-24));
	}
}

//
//	returns the value of the input pin (but does not shift it!)
//
//	note: the most efficient code is generated when testing the return value against 0.
//
static inline unsigned char input_test(unsigned char pin)
{
#ifdef NOTDEF
	if (pin < 8) {
		return _input_test(PINA,pin);
	} else
#endif
	if (pin >= 8 && pin < 16) {
		return _input_test(PINB,(pin-8));
	} else if (pin >= 16 && pin < 24) {
		return _input_test(PINC,(pin-16));
	} else if (pin >= 24 && pin < 32) {
		return _input_test(PIND,(pin-24));
	} else {
		return 0;	/* notreached */
	}
}

