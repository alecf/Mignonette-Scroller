/*
 *	miggl-private.h - Mignonette Game Library - internal (private) definitions (not part of API)
 *
 *	author(s): rolf van widenfelt (rolfvw at pizzicato dot com) (c) 2008 - Some Rights Reserved
 *
 *	author(s): mitch altman (c) 2008 - Some Rights Reserved
 *
 *	Note: This source code is licensed under a Creative Commons License, CC-by-nc-sa.
 *		(attribution, non-commercial, share-alike)
 *  	see http://creativecommons.org/licenses/by-nc-sa/3.0/ for details.
 *
 *	revision history:
 *
 *	- may 24, 2008 - rolf
 *		move MIN_NOTE constant to miggl.h.
 *
 *	- may 22, 2008 - rolf
 *		add another octave of notes, C3 to B3.  (note: we adjust MIN_NOTE constant!)
 *
 *	- may 18, 2008 - rolf
 *		cleanup tempo constants (used in DurTab array)
 *
 *	- may 16, 2008 - rolf
 *		more hacking on this...
 *
 *	- may 13, 2008 - rolf
 *		created (based on Mitch's audio.c and audio.h of 5/2)
 *
 *
 */


/* private audio-related defs */


// this is the size of all wave tables (in bytes) - seriously, don't change this!
#define WTABSIZE 32

#define TEMPOCONST 		1200000						// 20,000Hz * 60 sec

#define DEFAULTTEMPO	120.0						// default tempo in BPM (usually 75.0)

#define TEMPOBEAT		(TEMPOCONST/DEFAULTTEMPO)	// calculates duration value for a quarter note (1 beat)

#define NOTE_SEP 200			// length of small pause at end of each note (to differentiate each new note)


// fixed point number -- the integer part is as expected, the fractional part is a number divided by 256
struct fixedPtNum {
    uint8_t integ;  // left of the "decimal point"
    uint8_t fract;  // right of the "decimal point" up to 255 (up to but not including 256, which represents "1") -- "integ" actually represents "integ"/256
};


// XXX fix.. these should be hidden (static) inside miggl.c
extern uint16_t NoteTab[];
extern uint16_t DurTab[];


//
// convert standard note value into a "delta" for stepping through the wavetable.
// standard note values (e.g. N_C4 for C4, middle C) are used in the array passed to playsong().
//
// note: we use MIN_NOTE constant here to save bytes in NoteTab table.
//
#define GETNOTEDELTA(note)		(NoteTab[note-MIN_NOTE])

//
// convert standard duration constants (e.g. N_QUARTER) into actual ticks used by audio code
//
#define GETDURATION(dur)		(DurTab[dur-1])
