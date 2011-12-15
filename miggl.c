/*
 *	miggl.c - Mignonette Game Library, v0.94
 *
 *	author(s): rolf van widenfelt (rolfvw at pizzicato dot com) (c) 2008 - Some Rights Reserved
 *
 *	author(s): mitch altman (c) 2008 - Some Rights Reserved
 *
 *	author(s): jegge (c) 2010 - Some Rights Reserved
 *
 *	Note: This source code is licensed under a Creative Commons License, CC-by-nc-sa.
 *		(attribution, non-commercial, share-alike)
 *  	see http://creativecommons.org/licenses/by-nc-sa/3.0/ for details.
 *
 *
 *	hardware setup:
 *		- Mignonette v1.0
 *
 *	TODO:
 *
 *	- clean up initialization.. there should be one function miggl_init() or something like that.
 *		clean up global vars that shouldn't be exposed too.
 *
 *	- settempo() NYI
 *
 *	- really need to get rid of 48 entry duration table
 *		(use another counter and only re-calculate the 1/48 entry when tempo changes)
 *
 *	- could move wavetables into program memory (to save RAM)
 *
 *	- (as of may 17) do_audio_isr takes about 40-44% of the ISR's full duty cycle.
 *		the display part takes an additional 12-14%.
 *		tuning opportunity!
 *
 *
 *	revision history:
 *
 *  - jan 28, 2010 - jegge
 *		added random number generator
 *		minor bugfix in do_audio_isr()
 *		added songloop feature, so that a song is automatically repeated over and over
 *		added adsr envelope (fast version :))
 *		removed the notesep hack, with the envelope this gives clicking artifacts
 *		added a initmiggl function, so that only one function is needed for initialization
 *		moved delay_ functions from munch into the lib, but renamed them to sleep_* for compatibility
 *
 *	- apr 12, 2009 - rolf
 *		add readpixel() function.  up revision number to v0.93.
 *
 *	- may 26, 2008 - rolf
 *		minor comments/cleanup.
 *
 *	- may 24, 2008 - rolf
 *		call this version 0.92.
 *
 *	- may 22, 2008 - rolf
 *		add another octave of notes, C3 to B3.
 *
 *	- may 18, 2008 - rolf
 *		minor cleanup & comments.
 *
 *	- may 17, 2008 - rolf
 *		implement setwavetable() and add WT_SINE and WT_SQUARE choices.
 *		note: each table uses 32 bytes of RAM!
 *
 *		also, try making PWMval not volatile, then examine code gen... (hmmm, no diff).
 *
 *	- may 16, 2008 - rolf
 *		continue hacking audio code... playsong() now seems to work!
 *		the bulk of Mitch's audio ISR code remains intact.
 *
 *	- may 13, 2008 - rolf
 *		attempt to integrate Mitch's audio code!
 *		it looks like some API adjustments are needed in playsong(), etc.
 *		made a separate do_audio_isr() function to keep the code intact.
 *		this will eventually need to be merged into the ISR for efficiency.
 *
 *	- apr 27, 2008 - rolf
 *		release under Creative Commons CC-by-nc-sa license.
 *
 *	- apr 23, 2008 - rolf
 *		trying to add button event code.
 *
 *	- apr 19, 2008 - rolf
 *		add stubs for audio API.
 *
 *	- apr 18, 2008 - rolf
 *		basic gfx functionality works.
 *		now, move more low level functions (like avrinit) into here.
 *
 *	- apr 17, 2008 - rolf
 *		created.
 *
 *
 */

#include <inttypes.h>
#include <avr/io.h>			/* this takes care of definitions for our specific AVR */
#include <avr/pgmspace.h>	/* needed for printf_P, etc */
#include <avr/interrupt.h>	/* for interrupts, ISR macro, etc. */
#include <stdio.h>			// for sprintf, etc.
//#include <string.h>			// for strcpy, etc.

#include "uart.h"

// for _delay_us() macro  (note: this gets F_CPU define from uart.h)
#include <util/delay.h>

#include "mydefs.h"
#include "iodefs.h"

#include "miggl.h"
#include "miggl-private.h"


//
// globals for random number generator
//
static uint32_t RandomSeedA = 65537;
static uint32_t RandomSeedB = 12345;

// global graphics state
static uint8_t _CurColor = RED;


// globals for button handling
byte ButtonA;
byte ButtonB;
byte ButtonC;
byte ButtonD;
byte ButtonAEvent;
byte ButtonBEvent;
byte ButtonCEvent;
byte ButtonDEvent;


// globals for audio here

// sawtooth wavetable (TOP=49) (updated table from Mitch)
static uint8_t SawWtable[WTABSIZE] = {
  0,   2,   3,   5,
  6,   8,   9,  11,
 13,  14,  16,  17,
 19,  21,  22,  24,
 25,  27,  28,  30,
 32,  33,  35,  36,
 38,  40,  41,  43,
 44,  46,  47,  49,
};


// sinewave wavetable (TOP=49)
static uint8_t SineWtable[WTABSIZE] = {
  25, 29, 34, 38,
  42, 45, 47, 49,
  49, 49, 47, 45,
  42, 38, 34, 29,
  25, 20, 15, 11,
   7,  4,  2,  0,
   0,  0,  2,  4,
   7, 11, 15, 20,
};

// squarewave wavetable (TOP=49)
static uint8_t SquareWtable[WTABSIZE] = {
  0,   0,   0,   0,
  0,   0,   0,   0,
  0,   0,   0,   0,
  0,   0,   0,   0,
 49,  49,  49,  49,
 49,  49,  49,  49,
 49,  49,  49,  49,
 49,  49,  49,  49,
};


// globals for display/refresh here:

static volatile uint8_t Rcount = 20;


volatile uint8_t Disp[10];		// the display buffer (7 x 5 pixels ==> 10 rows of 7 pixels each, right-justified)

volatile uint8_t		CurRow;		// next display buffer row (of 5) to display

volatile uint8_t 	SwapRelease;	// flag (1 bit)
volatile uint8_t	SwapCounter;
uint8_t				SwapInterval;


// globals for audio here

//const uint8_t* wavTables[];  // table of addresses of different waveform tables (SINE, SAW, TRIANGLE, SQUARE, WEIRD)
uint8_t* wavPtr;                    // this points to the currently active waveform

uint16_t Wdur;        // duration for playing notes (these are in units of 50usec) -- initialize for 75 bpm (beats per minute)
//uint16_t Wnote_sep;   // small pause at end of each note (these are in units of 50usec)

uint16_t DurTab[];    // table of durations for notes to play (48 durations)

uint8_t* songPtr;				// this points into to the current song table
uint8_t* songBeginPtr;			// this points to the begin of the current song table
uint8_t  SongLoopFlag;			// if != 0, the song will be looped forever

volatile uint8_t CurNote;         // keeps track of note to play next time through the ISR

volatile uint8_t SongPlayFlag; // song play flag is 0 when not playing a song from song table, 1 while playing a song

int PWMval;           // this is the value that goes into 0CR1A (initialized to first value in wave table)

// WtabCount acts as a pointer through the wavetable as if there were a continuous wavetable, rather than just 32 discreet bytes
// WtabDelta is the amount to increment the WtabCount to get the next value from the wavetable
// fixed point number -- the integer part is as expected, the fractional part is a number divided by 256
//
// XXX note: should these be volatile?  -rolf
//
struct fixedPtNum WtabDelta;  // with this version of firmware we're limited to values between 1.000 and 1.996 (integ part always = 1)
struct fixedPtNum WtabCount;

volatile uint8_t EnvelopeA; // represents 1/256 of the overall length
volatile uint8_t EnvelopeD; // represents 1/256 of the overall length
volatile uint8_t EnvelopeS; // the fixed level of the sustain, must be >= 0 and < 63
volatile uint8_t EnvelopeR; // represents 1/256 of the overall length

volatile uint16_t EnvPointStartAttack; 	// the position of the note where the attack begins
volatile uint16_t EnvPointStartDecay; 	// the position of the note where the decay begins
volatile uint16_t EnvPointStartSustain; // the position of the note where the sustain begins
volatile uint16_t EnvPointStartRelease; // the position of the note where the release begins

struct fixedPtNum EnvValue;	// the current value of the envelope
struct fixedPtNum EnvDelta;	// the delta of the envelope (steepness)

//
// Random number generator functions
//

//
// calculates the next seeds and returns a "random" value between 0 and max
//
uint32_t nextrandom (uint32_t max) {
	RandomSeedA = 36969 * (RandomSeedA & 65535) + (RandomSeedA >> 16);
	RandomSeedB = 18000 * (RandomSeedB & 65535) + (RandomSeedB >> 16);
 	return ((RandomSeedA << 16) + RandomSeedB) % max;
}

//
// initializes the second random seed by adding the contents of the ram to it, since
// this is in an undefined state after booting.
//
void initrandom (void) {
	uint16_t *addr = 0;
	for (addr = 0; addr < (uint16_t*)0xFFFF; addr++)
		RandomSeedB += (*addr);
}


//
// audio portion of timer ISR
//
// (based on Mitch's ISR code from mig-testrefresh.c of 5/2/2008)
//
void do_audio_isr(void)
{
    uint8_t WtabVal1;   // two values from the wavetable between which we will interpolate
    uint8_t WtabVal2;
    uint16_t Wptr1;     // pointer to first value in wavetable
    uint16_t Wptr2;     // pointer to second value in wavetable
    uint16_t temp;
    int16_t tmpEnv;		// temp value for envelope calculation

    // The PWM value is loaded into the timer compare register at the beginning of the ISR if we are playing a song.
    // This PWM value was calculated in the previous pass through the ISR.

    // turn off audio if we have played the last note in the song table in the last pass through the ISR
    if ( CurNote == N_END ) {                  // if we reached the end of the song table

    	if ((SongLoopFlag != 0) && (songBeginPtr != NULL)) { // if the want to loop the song,
			playsong(songBeginPtr);							 // restart it.
			return;
		} // if not, ...

        SongPlayFlag = 0;                 // stop playing song when reach end of song table
        TCCR1A &= ~_BV(COM1A1);           // turn off audio by turning off compare
        //CurNote = 0;
    }

    // if we are playing a song, then calculate the PWM value to play the next time we get into the ISR
    if (SongPlayFlag) {          // only handle audio if we're playing a song (SongPlayFlag is set by main to start playing audio, and it is cleared by ISR when all events in active song table are completed)

        // if the Note to play is a Rest, then turn the speaker off
        if ( CurNote == N_REST )
            TCCR1A &= ~_BV(COM1A1);  // turn off audio by turning off compare
        // otherwise, start playing the note by putting the PWM value in the timer compare register, and turing on the speaker
        else {
            TCCR1A |= _BV(COM1A1);   // make sure audio is turned on by turning on compare reg
            OCR1A = PWMval;          // set the PWM time to next value (that was calculated on the previous pass through the ISR)
        }

        // calculate the next PWM value (this value will be used next time we get a timer interrrupt)

        // first, get the two values from the wavetable that we'll interpolating between
        Wptr2 = WtabCount.integ + WtabDelta.integ;
        temp = WtabCount.fract + WtabDelta.fract;
        if ( temp >= 256) Wptr2 += 1;   // if both fractional parts add to 1 or more, get next byte in wavetable for Val2
        if ( temp > 0) Wptr2 += 1;      // if there is a fractional part, get next byte in wavetable for Val2
        Wptr1 = Wptr2 - 1;              // the first value is always the byte before the second value
        if ( Wptr2 >= WTABSIZE) Wptr2 -= WTABSIZE;  // wrap around to the beginning of the wavetable if we reached the end of it
        if ( Wptr1 >= WTABSIZE) Wptr1 -= WTABSIZE;  // wrap around to the beginning of the wavetable if we reached the end of it
        WtabVal2 = wavPtr[Wptr2];       // get the second value from the wavetable
        WtabVal1 = wavPtr[Wptr1];       // get the first value from the wavetable

        // increment the Count by the Delta (fixed-point math)
        WtabCount.integ += WtabDelta.integ;
        temp = WtabCount.fract + WtabDelta.fract;  // we need to put this value in "temp" since "temp" is an int (16-bit value) and the fract parts of WtabCount and WtabDelta are 8-bit values
        // if the fractional part became 1 or beyond, then increment the integ part and correct the fractional part
        if ( temp >= 256 ) {                       // (256 is the equivalent of "1" for the fractional part)
            WtabCount.integ += 1;
            temp -= 256;
        }
        WtabCount.fract = temp;
        // if the counter is beyond the end of the table, then wrap it around to the beginning of the table
        if ( WtabCount.integ >= WTABSIZE) {
            WtabCount.integ -= WTABSIZE;
        }

        // now interpolate between the two values
        // NOTE: we are limited to WtabDelta between 1.0000 and 1.996 [ i.e. integ=1, fract=(0 to 255) ]
        // this calculates the following:
        //     if WtabVal2>WtabVal1:   PWMval = WtabVal1 + [(WtabVal2 - WtabVal1) * WtabCount]
        //     if WtabVal2<=WtabVal1:  PWMval = WtabVal1 - [(WtabVal1 - WtabVal2) * WtabCount]
        if (WtabVal2 > WtabVal1)
            temp = (WtabVal2 - WtabVal1) * WtabCount.fract;
        else
            temp = (WtabVal1 - WtabVal2) * WtabCount.fract;
        // round up if the fractional part of the result is 128 (80 hex) or more (i.e., "0.5" or more)
        if ( (temp & 0x00ff) < 0x0080 )
            temp = temp / 256;
        else
            temp = (temp / 256) + 1;
        // update PWMval
        if (WtabVal2 > WtabVal1)
            PWMval = WtabVal1 + temp;
        else
            PWMval = WtabVal1 - temp;
        if (PWMval < 0) PWMval = 0;    // PWM should never go below zero if the above math is good, but I put this check here just in case


		/// next step is calculating the envelope, based on the note duration count
		/// at special points of interest, we calculate the delta
		//  inbetween these points, we modifiy the envelope value, setting it or by adding or substracting the delta
		//Disp[9] = 0x00; // XXX Debug
		if (Wdur == EnvPointStartAttack) {
			// At the beginning, when starting the note, at the turnpoint before attack...
			// we calculate the delta for the phase using fixed point math
			tmpEnv = 64 * 256 / (EnvPointStartAttack - EnvPointStartDecay);
			EnvDelta.integ = tmpEnv / 256;
			EnvDelta.fract = tmpEnv - (EnvDelta.integ * 256);
			//Disp[9] = 0x40; // XXX Debug
		} else if (Wdur > EnvPointStartDecay) {
			// During Attack we add the delta to the value (fixed-point math)
			tmpEnv = EnvValue.fract + EnvDelta.fract;
			EnvValue.integ = EnvValue.integ + EnvDelta.integ;
			if (tmpEnv >= 256) {
				EnvValue.integ += 1;
				EnvValue.fract = tmpEnv - 256;
			} else
				EnvValue.fract = tmpEnv;
			//Disp[9] = 0x20; // XXX Debug
		} else if (Wdur == EnvPointStartDecay) {
			// At the turnpoint inbetween Attack and Decay, we calculate the next delta
			tmpEnv = ((64 - EnvelopeS) * 256) / (EnvPointStartDecay - EnvPointStartSustain);
			EnvDelta.integ = tmpEnv / 256;
			EnvDelta.fract = tmpEnv - (EnvDelta.integ * 256);
			//Disp[9] = 0x10; // XXX Debug
		} else if (Wdur > EnvPointStartSustain) {
			// During decay, we substract the delta from the value (fixed-point math)
			tmpEnv = EnvValue.fract - EnvDelta.fract;
			EnvValue.integ = EnvValue.integ - EnvDelta.integ;
			if (tmpEnv < 0) {
				EnvValue.integ -= 1;
				EnvValue.fract = tmpEnv + 256;
			} else
				EnvValue.fract = tmpEnv;
			//Disp[9] = 0x08; // XXX Debug
		} else if (Wdur > EnvPointStartRelease) {
			// From the turnpoint Decay to Sustain, and during Sustain, we just set the envelopes
			// value to the Sustain value
			EnvValue.integ = EnvelopeS;
			EnvValue.fract = 0;
			//Disp[9] = 0x04; // XXX Debug
		} else if (Wdur == EnvPointStartRelease) {
			// At the turnpoint from Sustain to Release, we calculate the next delta
			tmpEnv = (EnvelopeS * 256) / EnvPointStartRelease;
			EnvDelta.integ = tmpEnv / 256;
			EnvDelta.fract = tmpEnv - (EnvDelta.integ * 256);
			//Disp[9] = 0x02; // XXX Debug
		} else {
			// During releases, we substract the delta from the value  (fixed-point math)
			tmpEnv = EnvValue.fract - EnvDelta.fract;
			EnvValue.integ = EnvValue.integ - EnvDelta.integ;
			if (tmpEnv < 0) {
				EnvValue.integ -= 1;
				EnvValue.fract = tmpEnv + 256;
			} else
				EnvValue.fract = tmpEnv;
			//Disp[9] = 0x01; // XXX Debug
		}
		// the result is rounded ...
		if ((EnvValue.fract & 0x00FF) < 0x0080)
				temp = EnvValue.integ;
			else
				temp = EnvValue.integ + 1;

		// now we have the two parts that make out our sound - the PWMval, which contains the current "sample"
		// of our selected waveform, and temp, which contains the current value for our envelope.
		// The last step we need to do is to mix them.
		// NOTE: we use 64 steps of resolution in the envelope, since /64 is just bitshifting, which is consideratebly
		// faster that dividing through any "non-computer-friendly" value.
		PWMval = ((PWMval * temp) / 64) & 0xFF;


        // Wdur keeps track of the number of times through the ISR that we play a note (i.e., the duration of the sound)
        // If the duration is completed for playing this note (i.e., Wdur < 0), then we'll add a short pause after it to separate it from the next note
        if (Wdur > 0)                  // if the duration count is still above 0, then decrement it
            Wdur--;
        else {                         // else we have finished playing this note from the wavetable
            // start a slight pause after the note (to distinguish it from the note to follow)
            //if (Wnote_sep > 0) {                      // we'll keep playing no sound until we've gone through the ISR NOTE_SEP times, making a pause after playing the previously played note
            //    Wnote_sep--;
                //Disp[8] = 0x40;                     // XXX debug: turn on one pixel
                //DDRB &= ~_BV(1);                      // turn off SPKR (OC1A) port
            //}
            // if we're done with note separation pause, then set up the next note to play for the next time through the ISR
            //else {
            	uint16_t tmp;
				uint8_t note, dur;

                //Wnote_sep = NOTE_SEP;                 // reset note separation value
              	//DDRB |= _BV(1);                       // turn SPKR (OC1A) port back on
                //Disp[8] = 0x00;                     // XXX debug: turn off the one pixel

				// next time through the ISR we'll start playing the next note in the song table

				// note: this code is repeated inside playsong() - must match!!
				note = *songPtr++;
				tmp = GETNOTEDELTA(note);
				WtabDelta.integ = (uint8_t)((tmp >> 8) & 0xff);		// high byte
				WtabDelta.fract = (uint8_t)(tmp & 0xff);			// low byte
				dur = *songPtr++;
				CurNote = note;						// set 1st note to play, and
				Wdur = GETDURATION(dur);   			// its duration.

				EnvPointStartAttack  = Wdur;	// calculates the positions for the envelope parts
				EnvPointStartDecay   = EnvPointStartAttack - (Wdur / 256 * EnvelopeA);
				EnvPointStartSustain = EnvPointStartDecay  - (Wdur / 256 * EnvelopeD);
				EnvPointStartRelease = (Wdur / 256 * EnvelopeR);
				EnvValue.integ = 0;
				EnvValue.fract = 0;
				EnvDelta.integ = 0;
				EnvDelta.fract = 0;
           // }
        }
    }
}


ISR(TIMER1_OVF_vect)
{

	// first, handle audio
	do_audio_isr();


	// next, handle the display

	if (--Rcount == 0) {		// do we display a new row this time?  (only every 20 or so)
		Rcount = 20;

		//
		// we display green columns (5) followed by the red columns (5).
		// each will stay on for "Rcount" ticks (20 ticks is about 1ms).
		//
		switch (CurRow) {
			case 0:
				output_low(RC5);
				PORTD = Disp[0] | 0x80;		// note: keep PD7 high (pullup for SW4)
				output_high(GC1);
				break;

			case 1:
				output_low(GC1);
				PORTD = Disp[1] | 0x80;
				output_high(GC2);
				break;

			case 2:
				output_low(GC2);
				PORTD = Disp[2] | 0x80;
				output_high(GC3);
				break;

			case 3:
				output_low(GC3);
				PORTD = Disp[3] | 0x80;
				output_high(GC4);
				break;

			case 4:
				output_low(GC4);
				PORTD = Disp[4] | 0x80;
				output_high(GC5);
				break;

			case 5:
				output_low(GC5);
				PORTD = Disp[5] | 0x80;
				output_high(RC1);
				break;

			case 6:
				output_low(RC1);
				PORTD = Disp[6] | 0x80;
				output_high(RC2);
				break;

			case 7:
				output_low(RC2);
				PORTD = Disp[7] | 0x80;
				output_high(RC3);
				break;

			case 8:
				output_low(RC3);
				PORTD = Disp[8] | 0x80;
				output_high(RC4);
				break;

			case 9:
				output_low(RC4);
				PORTD = Disp[9] | 0x80;
				output_high(RC5);
				break;

		}	// switch


		CurRow++;
		if (CurRow >= 10) {
			CurRow = 0;
			if (--SwapCounter == 0) {			// we count down display cycles...
				SwapCounter = SwapInterval;
				SwapRelease = 1;				// now mark the end of the display cycle
			}
		}

	}
}


//
//
//	here, we start timer in "fast PWM" mode 14 (see waveform generation, pg 132 of atmega88 doc).
//
//
void start_timer1(void)
{

	// initialize ICR1, which sets the "TOP" value for the counter to interrupt and start over
	// note: value of 50-1 ==> 20khz (assumes 8mhz clock, prescaled by 1/8)
	//ICR1 = 50-1;
	ICR1 = 50-1;
	OCR1A = 25;

	//
	// start timer:
	// set fast PWM, mode 14
	// and set prescaler to system clock/8
	//

	TCCR1A = _BV(COM1A1) | _BV(WGM11);			// note: COM1A1 enables the compare match against OCR1A

	TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11);

	TIMSK1 |= _BV(TOIE1);		// enable timer1 overflow interrupt

}


/*
 *
 *	low level init needed for AVR.
 *
 */
void avrinit(void)
{

	initrandom(); 	// note: this is called at the very beginning, to ensure the highest possible
					// randomness of the ram

	// note: these MUST be in sync with actual hardware!  (also see iodefs.h)

	// note: DDR pins are set to "1" to be an output, "0" for input.

	//          76543210
	//PORTB = 0b00000101;		// initial: pullups on inputs
	//DDRB  = 0b11111010;		// inputs: SW1 (PB0), SW2 (PB2); outputs: SPKR (PB1), RC1-RC5 (PB3-PB7)
	PORTB = 0x05;			// (see above)
	DDRB  = 0xFA;			// (see above)

	//          76543210
	//PORTC = 0b00000001;		// initial: pullups on inputs
	//DDRC  = 0b11111110;		// inputs: SW3 (PC0); outputs: GC1-GC5 (PC1-PC5)
	PORTC = 0x01;		// (see above)
	DDRC  = 0xFE;		// (see above)

	//          76543210
	//PORTD = 0b10000000;		// initial: pullups on inputs
	//DDRD  = 0b01111111;		// inputs: SW4 (PD7) outputs: ROW1-ROW7 (PD0-PD6)

	PORTD = 0x80;		// (see above)
	DDRD  = 0x7F;		// (see above)


	sei();					// enable interrupts (individual interrupts still need to be enabled)
}


void button_init(void)
{
	ButtonA = 0;
	ButtonB = 0;
	ButtonC = 0;
	ButtonD = 0;
	ButtonAEvent = 0;
	ButtonBEvent = 0;
	ButtonCEvent = 0;
	ButtonDEvent = 0;
}


void poll_buttons(void)
{
	// clear the state of a button, if it has been released

	if (ButtonA) {
		if (!button_pressed(SW1)) {
			ButtonA = 0;
		}
	}
	if (ButtonB) {
		if (!button_pressed(SW2)) {
			ButtonB = 0;
		}
	}
	if (ButtonC) {
		if (!button_pressed(SW3)) {
			ButtonC = 0;
		}
	}
	if (ButtonD) {
		if (!button_pressed(SW4)) {
			ButtonD = 0;
		}
	}

}


//
// this watches for button "events" and performs actions accordingly.
//
void handlebuttons(void)
{

	if (!ButtonA && button_pressed(SW1)) {

		ButtonA = 1;

		// action
		ButtonAEvent = 1;

	} else if (!ButtonB && button_pressed(SW2)) {

		ButtonB = 1;

		// action
		ButtonBEvent = 1;

	} else if (!ButtonC && button_pressed(SW3)) {

		ButtonC = 1;

		// action
		ButtonCEvent = 1;

	} else if (!ButtonD && button_pressed(SW4)) {

		ButtonD = 1;

		// action
		ButtonDEvent = 1;

	} else {
		poll_buttons();
	}
}


/*
 *	wait (spin) until display cycle has finished
 *
 */
void swapbuffers(void)
{
	while (!SwapRelease) {		// spin until this flag is set
		NOP();
	}
	NOP();
	SwapRelease = 0;			// clear flag (for next time)
}

void initswapbuffers(void)
{
	SwapRelease = 0;
	SwapInterval = 1;
	SwapCounter = 1;
}

void swapinterval(uint8_t i)
{
	if (i != 0) {
		SwapInterval = i;
	}
}


void cleardisplay(void)
{
	uint8_t i;

	// initialize display buffer

	for (i = 0; i < 10; i++) {
		Disp[i] = 0x0;
	}

	//CurRow = 0;			// XXX needed??

	//Disp[0] = 0x40;		/* XXX debug: turn on just one pixel */
}


//
// set the current color (RED, GREEN, ...)
//
void setcolor(uint8_t c)
{
	_CurColor = 0x3 & c;
}


//
// get the current color (returns it).
//
uint8_t getcolor(void)
{
	return _CurColor;
}

//
// draw a point (single pixel) at coordinates (x y),
//	using the current color.
//
//	note: upper left is (0 0) and lower right is (6 4)
//
//
void drawpoint(uint8_t x, uint8_t y)
{
	uint8_t bits;

	if ((x < 7) && (y < 5)) {	// clipping
		bits = 0x40 >> x;
		if (_CurColor & 0x1) {	// red plane
			Disp[y+5] |= bits;
		} else {
			Disp[y+5] &= ~bits;
		}
		if (_CurColor & 0x2) {	// green plane
			Disp[y] |= bits;
		} else {
			Disp[y] &= ~bits;
		}
	}
}


//
// return the pixel at coordinates (x y).
//	the value returned is the color.
//	note: coordinates outside of the screen range will return BLACK (0).
//
uint8_t readpixel(uint8_t x, uint8_t y)
{
	uint8_t bits;
	uint8_t value;

	if ((x < 7) && (y < 5)) {	// clipping
		value = 0;
		bits = 0x40 >> x;
		if (Disp[y] & bits) {	// check green plane
			value |= GREEN;
		}
		if (Disp[y+5] & bits) {	// check red plane
			value |= RED;
		}
		return value;
	} else {
		return 0;
	}
}



//
//	draw a filled rectangle from (x1 y1) to (x2 y2)
//
//	XXX probably could be optimized more
//
void drawfilledrect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
	uint8_t bits;
	uint8_t x, y, tmp;

	if ((x1 < 7) && (y1 < 5) && (x2 < 7) && (y2 < 5)) {	// clipping
		if (x1 > x2) {
			tmp = x1;
			x1 = x2;
			x2 = tmp;
		}
		if (y1 > y2) {
			tmp = y1;
			y1 = y2;
			y2 = tmp;
		}
		for (y = y1; y <= y2; y++) {
			for (x = x1; x <= x2; x++) {
				bits = 0x40 >> x;
				if (_CurColor & 0x1) {	// red plane
					Disp[y+5] |= bits;
				} else {
					Disp[y+5] &= ~bits;
				}
				if (_CurColor & 0x2) {	// green plane
					Disp[y] |= bits;
				} else {
					Disp[y] &= ~bits;
				}
			}
		}
	}
}


// a simple API for making sounds.

void initaudio(void)
{
	// default wavetable (WT_SAWTOOTH)
	wavPtr = SawWtable;

	// default tempo
	//XXX
	SongLoopFlag = 0;
	SongPlayFlag = 0;
	PWMval = wavPtr[0];					// initialize to first entry of table

	EnvelopeA = 0; 	// these envelope settings should produce the same sound as the miggl-version
	EnvelopeD = 0; 	// without envelope
	EnvelopeS = 63;
	EnvelopeR = 0;
}


//
// sets tempo for playnote function.
// the default tempo is 72 beats per minute.
//
void settempo(byte bpm)
{
	// XXX NYI !!
}


//
// wavetables are just arrays of samples that produce waveforms.
// from the API all tables are just referenced by named constants.
// WT_SAWTOOTH is the default.
//
void setwavetable(byte wtable)
{
	if (wtable == WT_SINE) {
		wavPtr = SineWtable;
	} else if (wtable == WT_SAWTOOTH) {
		wavPtr = SawWtable;
	} else if (wtable == WT_SQUARE) {
		wavPtr = SquareWtable;
	}
}


//
// play a tone with pitch in Hz, and dur in ms.
// the current wavetable is used.
//
void playsound(int pitch, int dur)
{
	// XXX NYI !!
}


// play a tone with pitch "note" (uses predefined constants like C4 for middle C) and
// duration dur (predefined constants like N_QUARTER, etc.)
// the current wavetable is used.
//
// XXX NYI !!
void playnote(byte note, byte dur)
{}


//
// R2Nx - this converts a ratio (e.g. 1.000) into a standard note (e.g. a frequency),
// where "x" is the octave number (e.g. for middle C, x = 4).
//
// this is used to build the "note table" needed by the audio code.
//
#define R2N3(ratio)		(uint16_t)(ratio*64.0+0.5)

//
// convert ratio into "frequency" for audio code in ISR
//
#define R2N4(ratio)		(uint16_t)(ratio*128.0+0.5)

//
// octave higher than above (saves typing below)
//
#define R2N5(ratio)		(uint16_t)(ratio*256.0+0.5)

//
// table of "frequencies" for standard piano notes
//
// this table converts standard piano notes (e.g. N_C4) into 8.8 fixed point deltas
//	used in the wavetable synthesis code.
//
// note: currently, to make the math simpler, notes are transposed a bit.
//		for example, C5 is about 625 Hz when it really should be 523.251 Hz.  (off by about 3 half steps)
//		but, the final pitches should be relatively accurate because they are based on ratios
//
// also see GETNOTEDELTA() macro which references NoteTab.
//
uint16_t NoteTab[] = {
R2N3(1.000),	// N_C3 - C3 (1 octave below middle C)
R2N3(1.059),	// N_CS3
R2N3(1.122),	// N_D3
R2N3(1.189),	// N_DS3
R2N3(1.260),	// N_E3
R2N3(1.335),	// N_F3
R2N3(1.414),	// N_FS3
R2N3(1.498),	// N_G3
R2N3(1.587),	// N_GS3
R2N3(1.682),	// N_A3	- A3 (220 Hz)
R2N3(1.782),	// N_AS3
R2N3(1.888),	// N_B3

R2N4(1.000),	// N_C4 - C4 (middle C)
R2N4(1.059),	// N_CS4
R2N4(1.122),	// N_D4
R2N4(1.189),	// N_DS4
R2N4(1.260),	// N_E4
R2N4(1.335),	// N_F4
R2N4(1.414),	// N_FS4
R2N4(1.498),	// N_G4
R2N4(1.587),	// N_GS4
R2N4(1.682),	// N_A4	- A4 (440 Hz)
R2N4(1.782),	// N_AS4
R2N4(1.888),	// N_B4

R2N5(1.000),	// N_C5	- C5 (1 octave above middle C)
R2N5(1.059),	// N_CS5
R2N5(1.122),	// N_D5
R2N5(1.189),	// N_DS5
R2N5(1.260),	// N_E5
R2N5(1.335),	// N_F5
R2N5(1.414),	// N_FS5
R2N5(1.498),	// N_G5
R2N5(1.587),	// N_GS5
R2N5(1.682),	// N_A5	- A5 (880 Hz)
R2N5(1.782),	// N_AS5
R2N5(1.888),	// N_B5
R2N5(2.000),	// N_C6	- C6 (2 octaves above middle C)
};


//
// this table converts duration values (1..48) into the
// actual number of ticks used by the audio code.
// the table is loaded for a specific tempo (e.g. 75 bpm - beats [aka quarter note] per minute).
//
// design note:
//	by using 48 values, instead of a power of two like 16, we can represent triplets.
//	a quarter note (1 beat) is 12, an eighth note is 6, and an 8th triplet is 4.
//
//
// note: remember to subtract 1 before indexing this table with a standard duration (1..48)
//
// XXX need to fill in ALL entries in this table!
//	(just did the common ones, all the 0s are placeholders)
//
// also see GETDURATION() macro which references DurTab.
//
uint16_t DurTab[48] = {
0,0,TEMPOBEAT/4,TEMPOBEAT/3,0,TEMPOBEAT/2,
0,0,0,0,0,TEMPOBEAT,
0,0,0,0,0,0,
0,0,0,0,0,TEMPOBEAT*2,

0,0,0,0,0,0,
0,0,0,0,0,TEMPOBEAT*3,
0,0,0,0,0,0,
0,0,0,0,0,TEMPOBEAT*4,
};


//
// play a song, that is, a sequence of notes and durations.
// this is passed an array of bytes, which is filled with note/duration pairs,
// and must end with the byte N_END.
//
// XXX do we correctly handle the case where this is called when a song is currently playing?
//
void playsong(byte *songtable)
{
	uint16_t tmp;
	uint8_t note, dur;

	if (songtable == NULL) {		// error check
		return;
	}

	SongPlayFlag = 0;				// just in case a song is currently playing

	songPtr = songtable;			// set pointer to the song table array
	songBeginPtr = songtable;

	note = *songPtr++;
	if (note != N_END) {

		// note: this code is repeated inside ISR - must match!!
		tmp = GETNOTEDELTA(note);
		WtabDelta.integ = (uint8_t)((tmp >> 8) & 0xff);		// high byte
		WtabDelta.fract = (uint8_t)(tmp & 0xff);			// low byte
		dur = *songPtr++;
		CurNote = note;						// set 1st note to play, and
		Wdur = GETDURATION(dur);   			// its duration.

		EnvPointStartAttack  = Wdur;	// calculates the positions for the envelope parts
		EnvPointStartDecay   = EnvPointStartAttack - (Wdur / 256 * EnvelopeA);
		EnvPointStartSustain = EnvPointStartDecay  - (Wdur / 256 * EnvelopeD);
		EnvPointStartRelease = (Wdur / 256 * EnvelopeR);
		EnvValue.integ = 0;
		EnvValue.fract = 0;
		EnvDelta.integ = 0;
		EnvDelta.fract = 0;

		WtabCount.integ = 0;				// we will start playing from start of current wavetable
		WtabCount.fract = 0;
		PWMval = wavPtr[0];					// initialize to first entry of table
		SongPlayFlag = 1;					// start playing song
	}
}

//
// sets the song loop flag. If 0, the song will not be looped, if != 0, the song will be played
// on and on and on ... In case the song is currently looped and the flag is set to 0, the song
// will be finished.
//
void loopsong(uint8_t flag)
{
	SongLoopFlag = flag;
}

//
// this returns 1 if audio is playing, 0 otherwise.
//
byte isaudioplaying(void)
{
	return SongPlayFlag;
}


//
// this waits until audio (e.g. note or song) is finished, then returns.
//
void waitaudio(void)
{
	while (SongPlayFlag) {
		NOP();
	}

	return;
}

// sets the envelope parameters for the sound generator.
//
// a, d and r together must be smaller that 256, since the represent each a 1/256th of the note length
// if the sum is smaller, the rest will be used for the sustain
// s must be >= 0 and < 64
//
// if a note is currently played, these settings will take effect when the next note starts.
//
void setenvelope (uint8_t a, uint8_t d, uint8_t s, uint8_t r) {
	if ((a + d + r < 256) && (d < 64)) {
		EnvelopeA = a;
		EnvelopeD = d;
		EnvelopeS = s;
		EnvelopeR = r;
	}
}

//
// crude delay of 1 to 255 us -> Move this into the miggl-Lib?
//
void sleep_us (byte usec)
{
	usec++;
	while (--usec)
		_delay_us(1);		// get 1us delay from library macro (see <util/delay.h>)
}

//
// crude delay of 1 to 255 ms -> Move this into the miggl-Lib?
//
void sleep_ms (uint8_t ms)
{
	ms++;
	while (--ms)
		_delay_ms(1);		// get 1ms delay from library macro (see <util/delay.h>)
}


//
// crude "sleep" function for 0 to 255 seconds -> Move this into the miggl-Lib?
//
void sleep_sec (uint8_t sec)
{
	uint8_t i;
	for (i = 0; i < (sec << 0x02); i++)
		sleep_ms(250);
}


// for convinience: a single point of initialization

void initmiggl (void) {
	avrinit();
	initswapbuffers();
	swapinterval(10);		// note: display refresh is 100hz (lower number speeds up game)
	cleardisplay();
	start_timer1();			// this starts display refresh and audio processing
	button_init();
	initaudio();			// XXX eventually, we remove this!
}
