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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "error.h"
#include "dtypes.h"
#include "gr.h"
#include "mono.h"
#include "input.h"
#include "palette.h"
#include "game.h"
#include "gtimer.h"
#include "wiisys.h"

#include "newmenu.h"
#include "gamefont.h"
#include "network.h"
#include "iff.h"
#include "pcx.h"
#include "mem.h"
#include "screens.h"
#include "digi.h"

#include "cfile.h"
#include "compbit.h"
#include "songs.h"

#define ROW_SPACING 11
#define NUM_LINES 20			//19

ubyte fade_values[200] = { 1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,8,9,9,10,10,
11,11,12,12,12,13,13,14,14,15,15,15,16,16,17,17,17,18,18,19,19,19,20,20,
20,21,21,22,22,22,23,23,23,24,24,24,24,25,25,25,26,26,26,26,27,27,27,27,
28,28,28,28,28,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,30,30,30,30,30,
30,30,30,30,29,29,29,29,29,29,28,28,28,28,28,27,27,27,27,26,26,26,26,25,
25,25,24,24,24,24,23,23,23,22,22,22,21,21,20,20,20,19,19,19,18,18,17,17,
17,16,16,15,15,15,14,14,13,13,12,12,12,11,11,10,10,9,9,8,8,8,7,7,6,6,5,
5,4,4,3,3,2,2,1 };

extern ubyte *gr_bitblt_fade_table;

grs_font * header_font;
grs_font * title_font;
grs_font * names_font;

#define EXTRA_CREDIT_LINES		5
static const char *extra_lines[EXTRA_CREDIT_LINES] = {
	"*wii port programming",
	"tueidj",
	"",
	"*wii qa",
	"inimitable"
};
static int extra_index;

void credits_show()
{
	int i, j, l, done;
	CFILE * file;
	char buffer[NUM_LINES][80];
	grs_bitmap backdrop;
	ubyte backdrop_palette[768];
	int pcx_error;
	int buffer_line = 0;
	fix last_time;
	fix time_delay = 4180;			// ~ F1_0 / 12.9
	int first_line_offset,extra_inc=0;
	int have_bin_file = 0;
	char * tempp;

	extra_index = 0;

	set_screen_mode(SCREEN_MENU);

	// Clear out all tex buffer lines.
	for (i=0; i<NUM_LINES; i++ ) buffer[i][0] = 0;

	have_bin_file = 0;
	file = cfopen( "credits.tex", "rb" );
	if (file == NULL) {
		file = cfopen("credits.txb", "rb");
		if (file == NULL)
			Error("Missing CREDITS.TEX and CREDITS.TXB file\n");
		have_bin_file = 1;
	}

	gr_use_palette_table( "credits.256" );
	header_font = gr_init_font( "font1-1.fnt" );
	title_font = gr_init_font( "font2-3.fnt" );
	names_font = gr_init_font( "font2-2.fnt" );
	backdrop.bm_data=NULL;
	pcx_error = pcx_read_bitmap("stars.pcx",&backdrop,grd_curcanv->cv_bitmap.bm_type,backdrop_palette);
	if (pcx_error != PCX_ERROR_NONE)		{
		cfclose(file);
		return;
	}

	songs_play_song( SONG_CREDITS, 0 );

	gr_remap_bitmap_good( &backdrop,backdrop_palette, -1, -1 );

	gr_set_current_canvas(NULL);
	gr_bitmap(0,0,&backdrop);
	bitblt_to_screen();
	gr_palette_fade_in( gr_palette, 32, 0 );

	key_flush();
	last_time = timer_get_fixed_seconds();
	done = 0;
	first_line_offset = 0;
	while( 1 )	{
		int k;

		do {
			buffer_line = (buffer_line+1) % NUM_LINES;
			if (cfgets( buffer[buffer_line], 80, file ))	{
				char *p;
				if (have_bin_file) {				// is this a binary tbl file
					for (i = 0; i < strlen(buffer[buffer_line]) - 1; i++) {
						encode_rotate_left(&(buffer[buffer_line][i]));
						buffer[buffer_line][i] ^= BITMAP_TBL_XOR;
						encode_rotate_left(&(buffer[buffer_line][i]));
					}
				}
				p = strchr(&buffer[buffer_line][0],'\n');
				if (p) *p = '\0';
			} else if (extra_index<EXTRA_CREDIT_LINES) {
				strcpy(buffer[buffer_line], extra_lines[extra_index++]);
			} else	{
				//fseek( file, 0, SEEK_SET);
				buffer[buffer_line][0] = 0;
				done++;
			}
		} while (extra_inc--);
		extra_inc = 0;

		for (i=0; i<ROW_SPACING; i++ )	{
			int y;

			y = first_line_offset - i;

			gr_bitmap(0,0,&backdrop);
			for (j=0; j<NUM_LINES; j++ )	{
				char *s;

				l = (buffer_line + j + 1 ) %  NUM_LINES;
				s = buffer[l];

				if ( s[0] == '!' ) {
					s++;
				} else if ( s[0] == '$' )	{
					grd_curcanv->cv_font = header_font;
					s++;
				} else if ( s[0] == '*' )	{
					grd_curcanv->cv_font = title_font;
					s++;
				} else
					grd_curcanv->cv_font = names_font;

				gr_bitblt_fade_table = fade_values;

				tempp = strchr( s, '\t' );
				if ( tempp )	{
					int w, h, aw;
					*tempp = 0;
					gr_get_string_size( s, &w, &h, &aw );
					gr_printf( (160-w)/2, y, s );
					gr_get_string_size( &tempp[1], &w, &h, &aw );
					gr_printf( 160+((160-w)/2), y, &tempp[1] );
					*tempp = '\t';
				} else {
					gr_printf( 0x8000, y, s );
				}
				gr_bitblt_fade_table = NULL;
				if (buffer[l][0] == '!')
					y += ROW_SPACING/2;
				else
					y += ROW_SPACING;
			}

			bitblt_to_screen();
			while( timer_get_fixed_seconds() < last_time+time_delay );
			last_time = timer_get_fixed_seconds();


			k = key_inkey();

			#ifndef NDEBUG
			if (k == KEY_BACKSP) {
				Int3();
				k=0;
			}
			#endif

//			{
//				fix ot = time_delay;
//				time_delay += (keyd_pressed[KEY_X] - keyd_pressed[KEY_Z])*100;
//				if (ot!=time_delay)	{
//					mprintf( (0, "[%x] ", time_delay ));
//				}
//			}

			if (k == KEY_PRINT_SCREEN) {
				save_screen_shot(0);
				k = 0;
			}

			if ((k>0)||(done>NUM_LINES))	{
					gr_close_font(header_font);
					gr_close_font(title_font);
					gr_close_font(names_font);
					gr_palette_fade_out( gr_palette, 32, 0 );
					gr_use_palette_table( "palette.256" );
					myfree(backdrop.bm_data);
					cfclose(file);
					songs_play_song( SONG_TITLE, 1 );
					return;
			}
		}

		if (buffer[(buffer_line + 1 ) %  NUM_LINES][0] == '!') {
			first_line_offset -= ROW_SPACING-ROW_SPACING/2;
			if (first_line_offset <= -ROW_SPACING) {
				first_line_offset += ROW_SPACING;
				extra_inc++;
			}
		}
	}

}

