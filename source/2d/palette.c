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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "dtypes.h"
#include "mem.h"
#include "gr.h"
#include "fix.h"
#include "error.h"
#include "wiisys.h"
#include "palette.h"
#include "cfile.h"

extern int gr_installed;

ubyte gr_palette[256*3];
ubyte gr_current_pal[256*3];
ubyte gr_fade_table[256*34];
ubyte gr_debug_mode = 0;			// reverse white and black for debugging
ubyte gr_mac_gamma[64];
double gamma_corrections[9] = {1.4, 1.3, 1.2, 1.1, 1.0, 0.9, 0.8, 0.7, 0.6};


ubyte gr_palette_gamma = 4;
int gr_palette_gamma_param = 4;
ubyte gr_palette_faded_out = 1;

void gr_build_mac_gamma(double correction)
{
	int	i;

	for (i = 0; i < 64; i++)
		gr_mac_gamma[i] = MAX(0, MIN(255, (pow(i / 63.0, correction) * 255)));
}

void gr_palette_set_gamma( int gamma )
{
	gamma = MAX(0, MIN(8, gamma));

	if (gr_palette_gamma_param != gamma )	{
		gr_palette_gamma_param = gamma;
		gr_palette_gamma = gamma;
		gr_build_mac_gamma(gamma_corrections[gr_palette_gamma]);
		if (!gr_palette_faded_out)
			gr_palette_load( gr_palette );
	}
}

int gr_palette_get_gamma()
{
	return gr_palette_gamma_param;
}

void gr_use_palette_table( char * filename )
{
	CFILE *fp;
	int i;

	fp = cfopen( filename, "rb" );
	if ( fp==NULL)
		Error("Can't open palette file <%s> which is not in the current dir.",filename);

	cfread( gr_palette, 256*3, 1, fp );
	cfread( gr_fade_table, 256*GR_FADE_LEVELS, 1, fp );
	cfclose(fp);

	// This is the TRANSPARENCY COLOR
	for (i=0; i<GR_FADE_LEVELS; i++ )	{
		gr_fade_table[i*256+255] = TRANSPARENCY_COLOR;
	}

// swap colors 0 and 255 of the palette along with fade table entries

#ifdef SWAP_0_255
	for (i = 0; i < 3; i++) {
		c = gr_palette[i];
		gr_palette[i] = gr_palette[765+i];
		gr_palette[765+i] = c;
	}

	for (i = 0; i < GR_FADE_LEVELS * 256; i++) {
		if (gr_fade_table[i] == 0)
			gr_fade_table[i] = 255;
	}
	for (i=0; i<GR_FADE_LEVELS; i++)
		gr_fade_table[i*256] = TRANSPARENCY_COLOR;
#endif
}

#define SQUARE(x) ((x)*(x))

#define	MAX_COMPUTED_COLORS	32

int	Num_computed_colors=0;

color_record Computed_colors[MAX_COMPUTED_COLORS];

//	Add a computed color (by gr_find_closest_color) to list of computed colors in Computed_colors.
//	If list wasn't full already, increment Num_computed_colors.
//	If was full, replace a random one.
void add_computed_color(int r, int g, int b, int color_num)
{
	int	add_index;

	if (Num_computed_colors < MAX_COMPUTED_COLORS) {
		add_index = Num_computed_colors;
		Num_computed_colors++;
	} else
		add_index = (d_rand() * MAX_COMPUTED_COLORS) >> 15;

	Computed_colors[add_index].r = r;
	Computed_colors[add_index].g = g;
	Computed_colors[add_index].b = b;
	Computed_colors[add_index].color_num = color_num;
}

void init_computed_colors(void)
{
	int	i;

	for (i=0; i<MAX_COMPUTED_COLORS; i++)
		Computed_colors[i].r = 255;		//	Make impossible to match.
}

int gr_find_closest_color( int r, int g, int b )
{
	int i, j;
	int best_value, best_index, value;

	if (Num_computed_colors == 0)
		init_computed_colors();

	//	If we've already computed this color, return it!
	for (i=0; i<Num_computed_colors; i++)
		if (r == Computed_colors[i].r)
			if (g == Computed_colors[i].g)
				if (b == Computed_colors[i].b) {
					if (i > 4) {
						color_record	trec;
						trec = Computed_colors[i-1];
						Computed_colors[i-1] = Computed_colors[i];
						Computed_colors[i] = trec;
						return Computed_colors[i-1].color_num;
					}
					return Computed_colors[i].color_num;
				}

	r &= 63;
	g &= 63;
	b &= 63;

	best_value = SQUARE(r-gr_palette[0])+SQUARE(g-gr_palette[1])+SQUARE(b-gr_palette[2]);
	best_index = 0;
	if (best_value==0) {
		add_computed_color(r, g, b, best_index);
 		return best_index;
	}
	j=0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i=1; i<254; i++ )	{
		j += 3;
		value = SQUARE(r-gr_palette[j])+SQUARE(g-gr_palette[j+1])+SQUARE(b-gr_palette[j+2]);
		if ( value < best_value )	{
			if (value==0) {
				add_computed_color(r, g, b, i);
				return i;
			}
			best_value = value;
			best_index = i;
		}
	}
	add_computed_color(r, g, b, best_index);
	return best_index;
}

int gr_find_closest_color_15bpp( int rgb )
{
	return gr_find_closest_color( ((rgb>>10)&31)*2, ((rgb>>5)&31)*2, (rgb&31)*2 );
}

int gr_find_closest_color_current( int r, int g, int b )
{
	int i, j;
	int best_value, best_index, value;

	r &= 63;
	g &= 63;
	b &= 63;

	best_value = SQUARE(r-gr_current_pal[0])+SQUARE(g-gr_current_pal[1])+SQUARE(b-gr_current_pal[2]);
	best_index = 0;
	if (best_value==0)
 		return best_index;

	j=0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i=1; i<254; i++ )	{
		j += 3;
		value = SQUARE(r-gr_current_pal[j])+SQUARE(g-gr_current_pal[j+1])+SQUARE(b-gr_current_pal[j+2]);
		if ( value < best_value )	{
			if (value==0)
				return i;
			best_value = value;
			best_index = i;
		}
	}
	return best_index;
}

static int last_r=0, last_g=0, last_b=0;

void gr_palette_step_up( int r, int g, int b )
{
	int i;
	ubyte *p;
	int temp;
	color_record colors[256];

	if (gr_palette_faded_out) return;

	if ( (r==last_r) && (g==last_g) && (b==last_b) ) return;

	last_r = r;
	last_g = g;
	last_b = b;

	p=gr_palette;

	for (i=0; i<256; i++ )	{
		colors[i].color_num = i;

		temp = (int)(*p++) + r;
		temp = MAX(0, MIN(63, temp));
		colors[i].r = gr_mac_gamma[temp];

		temp = (int)(*p++) + g;
		temp = MAX(0, MIN(63, temp));
		colors[i].g = gr_mac_gamma[temp];

		temp = (int)(*p++) + b;
		temp = MAX(0, MIN(63, temp));
		colors[i].b = gr_mac_gamma[temp];
	}
	SetEntries(0, 255, colors);
}

void gr_palette_clear()
{
	int i;
	color_record colors[256];

	for (i = 0; i < 256; i++) {
		colors[i].color_num = i;
		colors[i].r = 0;
		colors[i].g = 0;
		colors[i].b = 0;
	}
	SetEntries(0, 255, colors);
	gr_palette_faded_out = 1;
}

void gr_palette_load( ubyte *pal )
{
	int i, j;
	color_record colors[256];

	for (i=0; i<768; i++ )
		gr_current_pal[i] = MIN(63, pal[i]);

	for (i = 0, j = 0; j < 256; j++) {
		colors[j].color_num = j;
		colors[j].r = gr_mac_gamma[gr_current_pal[i++]];
		colors[j].g = gr_mac_gamma[gr_current_pal[i++]];
		colors[j].b = gr_mac_gamma[gr_current_pal[i++]];
	}
	SetEntries(0, 255, colors);
	gr_palette_faded_out = 0;
	init_computed_colors();
}

static fix fade_palette[768];
static fix fade_palette_delta[768];

int gr_palette_fade_out(ubyte *pal, int nsteps, int allow_keys )
{
	int i,j, k;
	color_record colors[256];

	if (gr_palette_faded_out) return 0;

	for (i=0; i<768; i++ )	{
		fade_palette[i] = i2f(pal[i]);
		fade_palette_delta[i] = fade_palette[i] / nsteps;
	}

	for (j=0; j<nsteps; j++ )	{
		for (i=0, k = 0; k < 256; k++)	{
			colors[k].color_num = k;

			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			colors[k].r = gr_mac_gamma[(f2i(fade_palette[i++]))];

			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			colors[k].g = gr_mac_gamma[(f2i(fade_palette[i++]))];

			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			colors[k].b = gr_mac_gamma[(f2i(fade_palette[i++]))];
		}
		SetEntries(0, 255, colors);
		bitblt_to_screen();
	}

	gr_palette_faded_out = 1;
	return 0;
}

int gr_palette_fade_in(ubyte *pal, int nsteps, int allow_keys)
{
	int i,j, k;
	ubyte c;
	color_record colors[256];

	if (!gr_palette_faded_out) return 0;

	for (i=0; i<768; i++ )	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = 0;
		fade_palette_delta[i] = i2f(pal[i]) / nsteps;
	}

	for (j=0; j<nsteps; j++ )	{
		for (i=0, k = 0; k<256; k++ )	{
			colors[k].color_num = k;

			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i]) )
				fade_palette[i] = i2f(pal[i]);
			c = f2i(fade_palette[i++]);
			c = MIN(63, c);
			colors[k].r = gr_mac_gamma[c];

			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i]) )
				fade_palette[i] = i2f(pal[i]);
			c = f2i(fade_palette[i++]);
			c = MIN(63, c);
			colors[k].g = gr_mac_gamma[c];

			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i]) )
				fade_palette[i] = i2f(pal[i]);
			c = f2i(fade_palette[i++]);
			c = MIN(63, c);
			colors[k].b = gr_mac_gamma[c];
		}
		SetEntries(0, 255, colors);
		bitblt_to_screen();
	}

	gr_palette_faded_out = 0;
	return 0;
}

void debug_video_mode()
{
	gr_debug_mode = 1;
}

void reset_debug_video_mode()
{
	gr_debug_mode = 0;
}

void gr_palette_read(ubyte * pal)
{
	memcpy(pal, gr_current_pal, 3*256);
}

void gr_make_cthru_table(ubyte * table, ubyte r, ubyte g, ubyte b )
{
	int i;
	ubyte r1, g1, b1;

	for (i=0; i<256; i++ )	{
		r1 = gr_palette[i*3+0] + r;
		if ( r1 > 63 ) r1 = 63;
		g1 = gr_palette[i*3+1] + g;
		if ( g1 > 63 ) g1 = 63;
		b1 = gr_palette[i*3+2] + b;
		if ( b1 > 63 ) b1 = 63;
		table[i] = gr_find_closest_color( r1, g1, b1 );
	}
}

