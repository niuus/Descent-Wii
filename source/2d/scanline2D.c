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

#include "mem.h"
#include "gr.h"
#include "grdef.h"

int Gr_scanline_darkening_level = GR_FADE_LEVELS;

void gr_linear_darken( ubyte * dest, int darkening_level, int count, ubyte * fade_table )
{
	int i;
	for (i=0; i<count; i++)
		dest[i] = fade_table[dest[i]+(darkening_level*256)];

}

void gr_uscanline( int x1, int x2, int y )
{
//	memset(DATA + ROWSIZE*y + x1, COLOR, x2-x1+0);
//
	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS )	{
		gr_linear_stosd( DATA + ROWSIZE*y + x1, COLOR, x2-x1+1);
	} else {
		gr_linear_darken( DATA + ROWSIZE*y + x1, Gr_scanline_darkening_level, x2-x1+1, gr_fade_table);
	}
}

void gr_scanline( int x1, int x2, int y )
{
	if ((y<0)||(y>MAXY)) return;

	if (x2 < x1 ) x2 ^= x1 ^= x2;

	if (x1 > MAXX) return;
	if (x2 < MINX) return;

	if (x1 < MINX) x1 = MINX;
	if (x2 > MAXX) x2 = MAXX;

//	memset(DATA + ROWSIZE*y + x1, COLOR, x2-x1+1);
//
	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS )	{
		gr_linear_stosd( DATA + ROWSIZE*y + x1, COLOR, x2-x1+1);
	} else {
		gr_linear_darken( DATA + ROWSIZE*y + x1, Gr_scanline_darkening_level, x2-x1+1, gr_fade_table);
	}
}

