/*
 *	miggl.h - Mignonette Game Library, v0.94 - definitions
 *
 *	author(s): rolf van widenfelt (rolfvw at pizzicato dot com) (c) 2008 - Some Rights Reserved
 *
 *	author(s): jegge (c) 2010 - Some Rights Reserved
 *
 *	Note: This source code is licensed under a Creative Commons License, CC-by-nc-sa.
 *		(attribution, non-commercial, share-alike)
 *  	see http://creativecommons.org/licenses/by-nc-sa/3.0/ for details.
 *
 *	revision history:
 *
 *  - jan 28, 2010 - jegge
 *		added random number generator
 *		added songloop feature, so that a song is automatically repeated over and over
 *		added adsr envelope (fast version :))
 *		added a initmiggl function, so that only one function is needed for initialization
 *
 *	- apr 12, 2009 - rolf
 *		add readpixel() function.
 *
 *	- may 24, 2008 - rolf
 *		move MIN_NOTE constant to here.
 *
 *	- may 22, 2008 - rolf
 *		add another octave of notes, C3 to B3.
 *			(note: need to adjust MIN_NOTE constant in miggl-private.h)
 *
 *	- may 18, 2008 - rolf
 *		minor cleanup & comments.
 *
 *	- may 17, 2008 - rolf
 *		add wavetable constants (WT_SINE, etc)
 *
 *	- may 13, 2008 - rolf
 *		add piano notes N_C4, etc.  add durations N_QUARTER, etc.
 *
 *	- apr 27, 2008 - rolf
 *		release under Creative Commons CC-by-nc-sa license.
 *
 *	- apr 19, 2008 - rolf
 *		track changes to miggl.c.
 *
 *	- apr 17, 2008 - rolf
 *		created.
 *
 *
 */


/* colors */
#define BLACK	0
#define RED		1
#define GREEN	2
#define YELLOW	3

/* display size (in pixels) */
#define XSCREEN 7
#define YSCREEN 5

/* notes (incomplete!) */
#define N_END	0
#define N_REST	255

#define N_C3	28		// C3 (1 octave below middle C)
#define N_CS3	29
#define N_D3	30
#define N_DS3	31
#define N_E3	32
#define N_F3	33
#define N_FS3	34
#define N_G3	35
#define N_GS3	36
#define N_A3	37		// A3 (220 Hz)
#define N_AS3	38
#define N_B3	39

#define N_C4	40		// C4 (middle C)
#define N_CS4	41
#define N_D4	42
#define N_DS4	43
#define N_E4	44
#define N_F4	45
#define N_FS4	46
#define N_G4	47
#define N_GS4	48
#define N_A4	49		// A4 (440 Hz)
#define N_AS4	50
#define N_B4	51

#define N_C5	52		// C5 (1 octave above middle C - 523.251 Hz)
#define N_CS5	53
#define N_D5	54
#define N_DS5	55
#define N_E5	56
#define N_F5	57
#define N_FS5	58
#define N_G5	59
#define N_GS5	60
#define N_A5	61
#define N_AS5	62
#define N_B5	63
#define N_C6	64

// always set to the lowest note!
#define MIN_NOTE	N_C3

#define N_16TH 		3
#define N_8TH 		6
#define N_QUARTER	12
#define N_HALF		24
#define N_WHOLE		48

// XXX need more...
#define N_HALF_DOT	36
#define N_8TH_TRIP 	4


/* wavetable choices - used with setwavetable() */
#define WT_SAWTOOTH		1
#define WT_SINE			2
#define WT_SQUARE		3


/* globals for buttons */
extern byte ButtonA;
extern byte ButtonB;
extern byte ButtonC;
extern byte ButtonD;
extern byte ButtonAEvent;
extern byte ButtonBEvent;
extern byte ButtonCEvent;
extern byte ButtonDEvent;


extern volatile uint8_t Disp[];		// XXX probably shouldn't access this!


/* graphics functions */

void swapbuffers(void);
void initswapbuffers(void);
void swapinterval(uint8_t i);
void cleardisplay(void);
void setcolor(uint8_t c);
void drawpoint(uint8_t x, uint8_t y);
uint8_t readpixel(uint8_t x, uint8_t y);
void drawfilledrect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);


/* button functions */

void button_init(void);
void poll_buttons(void);
void handlebuttons(void);


/* audio functions */

void initaudio(void);

//void playsound(int pitch, int dur);

void settempo(byte bpm);
void setwavetable(byte wtable);
void playnote(byte note, byte dur);
void playsong(byte *songtable);
void loopsong(uint8_t flag);

byte isaudioplaying(void);		// returns 1 if audio is playing, 0 otherwise
void waitaudio(void);			// waits until audio (e.g. note or song) is finished

void setenvelope (uint8_t a, uint8_t d, uint8_t s, uint8_t r);

/* sleep functions */
void sleep_us (byte usec);
void sleep_ms (uint8_t ms);
void sleep_sec (uint8_t sec);

/* random number generation */
uint32_t nextrandom (uint32_t max);

/* XXX stuff that probably shouldn't be here... */
void avrinit(void);
void start_timer1(void);

void initmiggl (void);
