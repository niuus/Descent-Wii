/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _DEMO_H
#define _DEMO_H

#include "types.h"
#include "vecmat.h"

#define DEMO_INACTIVE 0
#define DEMO_RECORDING 1
#define DEMO_PLAYBACK 2

#define MAX_DEMO_RECS 1000	// a recording is made once/second (search for 65536 in demo.c)

typedef struct _demorec {
	fix		time;
	fix		x,y,z;
	fixang	p,b,h;
	short		segnum;
//	short		specials;
} demorec;


extern int Demo_mode, Auto_demo, demo_loaded;

extern void start_demo_playback(void);
extern void start_demo_recording(void);
extern void record_demo_frame(void);

extern int get_demo_data(fix curtime, vms_vector *pos, vms_angvec *pbh, short *segnum, int *do_fire);

extern void demo_startup(void);

#endif
