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
#include "text.h"

#include "newmenu.h"
#include "gamefont.h"
#include "network.h"
#include "iff.h"
#include "pcx.h"
#include "mem.h"
#include "digi.h"
#include "gtimer.h"
#include "fileutil.h"

#include "multi.h"
#include "endlevel.h"
#include "screens.h"
#include "kconfig.h"
#include "player.h"
#include <dirent.h>
#include "wiisys.h"

#define TITLE_FONT  		(Gamefonts[GFONT_BIG_1])

#define SUBTITLE_FONT	(Gamefonts[GFONT_MEDIUM_3])
#define CURRENT_FONT  	(Gamefonts[GFONT_MEDIUM_2])
#define NORMAL_FONT  	(Gamefonts[GFONT_MEDIUM_1])
#define TEXT_FONT  		(Gamefonts[GFONT_MEDIUM_3])

#define CURSOR_STRING		"_"
static char NORMAL_CHECK_BOX[2];
static char CHECKED_CHECK_BOX[2];
static char NORMAL_RADIO_BOX[2];
static char CHECKED_RADIO_BOX[2];
static char SLIDER_LEFT[2];
static char SLIDER_RIGHT[2];
static char SLIDER_MIDDLE[2];
static char SLIDER_MARKER[2];

int Newmenu_first_time = 1;
//--unused-- int Newmenu_fade_in = 1;

typedef struct bkg {
	grs_canvas * menu_canvas;
	grs_bitmap * saved;			// The background under the menu.
	grs_bitmap * background;
} bkg;

grs_bitmap nm_background;

#define MESSAGEBOX_TEXT_SIZE 300		// How many characters in messagebox
#define MAX_TEXT_WIDTH 	200				// How many pixels wide a input box can be

extern void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

void newmenu_close()	{
	if ( nm_background.bm_data )
		myfree(nm_background.bm_data);
	Newmenu_first_time = 1;
}

void nm_draw_background1(char * filename)
{
	int pcx_error;
	grs_bitmap *bmp;
	int x, y;
	ubyte pal[768];

	gr_clear_canvas( BM_XRGB(0,0,0) );
	x = (grd_curcanv->cv_bitmap.bm_w - grd_curscreen->sc_w) / 2;
	y = (grd_curcanv->cv_bitmap.bm_h - grd_curscreen->sc_h) / 2;
	bmp = gr_create_sub_bitmap( &grd_curcanv->cv_bitmap, x, y, grd_curscreen->sc_w, grd_curscreen->sc_h );
	pcx_error = pcx_read_bitmap(filename,bmp,bmp->bm_type,pal);
	Assert(pcx_error == PCX_ERROR_NONE);

	gr_free_sub_bitmap(bmp);
}

void nm_draw_background(int x1, int y1, int x2, int y2 )
{
	int w,h;

	if (Newmenu_first_time)	{
		int pcx_error;
		ubyte newpal[768];
		atexit( newmenu_close );
		Newmenu_first_time = 0;

		nm_background.bm_data=NULL;
		pcx_error = pcx_read_bitmap("SCORES.PCX",&nm_background,BM_LINEAR,newpal);
		Assert(pcx_error == PCX_ERROR_NONE);

		gr_remap_bitmap_good( &nm_background, newpal, -1, -1 );

//  Assign character values to special characters;
		sprintf (NORMAL_RADIO_BOX, "%c", 127);
		sprintf (CHECKED_RADIO_BOX, "%c", 128);
		sprintf (NORMAL_CHECK_BOX, "%c", 129);
		sprintf (CHECKED_CHECK_BOX, "%c", 130);
		sprintf (SLIDER_LEFT, "%c", 131);
		sprintf (SLIDER_RIGHT, "%c", 132);
		sprintf (SLIDER_MIDDLE, "%c", 133);
		sprintf (SLIDER_MARKER, "%c", 134);
	}

	if ( x1 < 0 ) x1 = 0;
	if ( y1 < 0 ) y1 = 0;

	w = x2-x1+1;
	h = y2-y1+1;

	if ( w > nm_background.bm_w ) w = nm_background.bm_w;
	if ( h > nm_background.bm_h ) h = nm_background.bm_h;

	x2 = x1 + w - 1;
	y2 = y1 + h - 1;

	gr_bm_bitblt(w, h, x1, y1, 0, 0, &nm_background, &(grd_curcanv->cv_bitmap) );

	Gr_scanline_darkening_level = 2*7;

	gr_setcolor( BM_XRGB(0,0,0) );
	gr_urect( x2-5, y1+5, x2-5, y2-5 );
	gr_urect( x2-4, y1+4, x2-4, y2-5 );
	gr_urect( x2-3, y1+3, x2-3, y2-5 );
	gr_urect( x2-2, y1+2, x2-2, y2-5 );
	gr_urect( x2-1, y1+1, x2-1, y2-5 );
	gr_urect( x2+0, y1+0, x2-0, y2-5 );

	gr_urect( x1+5, y2-5, x2, y2-5 );
	gr_urect( x1+4, y2-4, x2, y2-4 );
	gr_urect( x1+3, y2-3, x2, y2-3 );
	gr_urect( x1+2, y2-2, x2, y2-2 );
	gr_urect( x1+1, y2-1, x2, y2-1 );
	gr_urect( x1+0, y2, x2, y2-0 );

	Gr_scanline_darkening_level = GR_FADE_LEVELS;
}

void nm_restore_background( int x, int y, int w, int h )
{
	int x1, x2, y1, y2;

	x1 = x; x2 = x+w-1;
	y1 = y; y2 = y+h-1;

	if ( x1 < 0 ) x1 = 0;
	if ( y1 < 0 ) y1 = 0;

	if ( x2 >= nm_background.bm_w ) x2=nm_background.bm_w-1;
	if ( y2 >= nm_background.bm_h ) y2=nm_background.bm_h-1;

	w = x2 - x1 + 1;
	h = y2 - y1 + 1;

	gr_bm_bitblt(w, h, x1, y1, x1, y1, &nm_background, &(grd_curcanv->cv_bitmap) );
}

// Draw a left justfied string
void nm_string( bkg * b, int w1,int x, int y, char * s )
{
	int w,h,aw;
	char *p,*s1;

	p = strchr( s, '\t' );
	if (p && (w1>0) )	{
		*p = '\0';
		s1 = p+1;
	}

	gr_get_string_size(s, &w, &h, &aw  );

	if (w1 > 0)
		w = w1;

	// CHANGED
	gr_bm_bitblt(b->background->bm_w-15, h, 5, y, 5, y, b->background, &(grd_curcanv->cv_bitmap) );
	//gr_bm_bitblt(w, h, x, y, x, y, b->background, &(grd_curcanv->cv_bitmap) );

	gr_string( x, y, s );

	if (p && (w1>0) )	{
		gr_get_string_size(s1, &w, &h, &aw  );

		gr_string( x+w1-w, y, s1 );

		*p = '\t';
	}

}


// Draw a slider and it's string
void nm_string_slider( bkg * b, int w1,int x, int y, char * s )
{
	int w,h,aw;
	char *p,*s1;

	p = strchr( s, '\t' );
	if (p)	{
		*p = '\0';
		s1 = p+1;
	}

	gr_get_string_size(s, &w, &h, &aw  );
	// CHANGED
	gr_bm_bitblt(b->background->bm_w-15, h, 5, y, 5, y, b->background, &(grd_curcanv->cv_bitmap) );
	//gr_bm_bitblt(w, h, x, y, x, y, b->background, &(grd_curcanv->cv_bitmap) );

	gr_string( x, y, s );

	if (p)	{
		gr_get_string_size(s1, &w, &h, &aw  );

		// CHANGED
		gr_bm_bitblt(w, 1, x+w1-w, y, x+w1-w, y, b->background, &(grd_curcanv->cv_bitmap) );
		// CHANGED
		gr_bm_bitblt(w, 1, x+w1-w, y+h-1, x+w1-w, y, b->background, &(grd_curcanv->cv_bitmap) );

		gr_string( x+w1-w, y, s1 );

		*p = '\t';
	}

}


// Draw a left justfied string with black background.
void nm_string_black( bkg * b, int w1,int x, int y, char * s )
{
	int w,h,aw;
	gr_get_string_size(s, &w, &h, &aw  );
	b = b;
	if (w1 == 0) w1 = w;

	gr_setcolor( BM_XRGB(0,0,0) );
	gr_rect( x, y, x+w1-1, y+h-1 );

	gr_string( x, y, s );
}


// Draw a right justfied string
void nm_rstring( bkg * b,int w1,int x, int y, char * s )
{
	int w,h,aw;
	gr_get_string_size(s, &w, &h, &aw  );
	x -= 6;

	if (w1 == 0) w1 = w;

	//mprintf( 0, "Width = %d, string='%s'\n", w, s );

	// CHANGED
	gr_bm_bitblt(w1, h, x-w1, y, x-w1, y, b->background, &(grd_curcanv->cv_bitmap) );

	gr_string( x-w, y, s );
}

//for text items, constantly redraw cursor (to achieve flash)
static void update_cursor( newmenu_item *item)
{
	int w,h,aw;
	fix time = timer_get_fixed_seconds();
	int x,y;
	char * text = item->text;

	Assert(item->type==NM_TYPE_INPUT_MENU || item->type==NM_TYPE_INPUT);

	while( *text )	{
		gr_get_string_size(text, &w, &h, &aw  );
		if ( w > item->w-10 )
			text++;
		else
			break;
	}
	if (*text==0)
		w = 0;
	x = item->x+w; y = item->y;

	if (time & 0x8000) {
		if (item->next_key != ' ') {
			char s[2];
			s[0] = item->next_key;
			s[1] = '\0';
			gr_string( x, y, s);
		}
		else
			gr_string(x, y, CURSOR_STRING);
	}
	else {
		gr_setcolor( BM_XRGB(0,0,0) );
		gr_rect( x, y, x+grd_curcanv->cv_font->ft_w-1, y+grd_curcanv->cv_font->ft_h-1 );
	}

}

void nm_string_inputbox( bkg *b, int w, int x, int y, char * text, int current)
{
	int w1,h1,aw;

	while( *text )	{
		gr_get_string_size(text, &w1, &h1, &aw  );
		if ( w1 > w-10 )
			text++;
		else
			break;
	}
	if ( *text == 0 )
		w1 = 0;
	nm_string_black( b, w, x, y, text );

	switch (current) {
		case ' ':
			gr_string(x+w1, y, CURSOR_STRING);
		case 0:
			break;
		default: {
			char s[2];
			s[0] = current;
			s[1] = '\0';
			gr_string( x+w1, y, s);
		}
	}
}

void draw_item( bkg * b, newmenu_item *item, int is_current )
{
	if (is_current)
		grd_curcanv->cv_font = CURRENT_FONT;
	else
		grd_curcanv->cv_font = NORMAL_FONT;

	switch( item->type )	{
	case NM_TYPE_TEXT:
		grd_curcanv->cv_font=TEXT_FONT;
		// fall through on purpose
	case NM_TYPE_MENU:
		nm_string( b, item->w, item->x, item->y, item->text );
		break;
	case NM_TYPE_SLIDER:	{
		int j;

		if (item->value < item->min_value) item->value=item->min_value;
		if (item->value > item->max_value) item->value=item->max_value;
		sprintf( item->saved_text, "%s\t%s", item->text, SLIDER_LEFT );
		for (j=0; j<(item->max_value-item->min_value+1); j++ )	{
			sprintf( item->saved_text, "%s%s", item->saved_text,SLIDER_MIDDLE );
		}
		sprintf( item->saved_text, "%s%s", item->saved_text,SLIDER_RIGHT );

		item->saved_text[item->value+1+strlen(item->text)+1] = SLIDER_MARKER[0];
		nm_string_slider( b, item->w, item->x, item->y, item->saved_text );
		}
		break;
	case NM_TYPE_INPUT_MENU:
		if ( item->group==0 )		{
			nm_string( b, item->w, item->x, item->y, item->text );
		} else {
			nm_string_inputbox( b, item->w, item->x, item->y, item->text, (is_current ? item->next_key:0) );
		}
		break;
	case NM_TYPE_INPUT:
		nm_string_inputbox( b, item->w, item->x, item->y, item->text, (is_current ? item->next_key:0) );
		break;
	case NM_TYPE_CHECK:
		nm_string( b, item->w, item->x, item->y, item->text );
		if (item->value)
			nm_rstring( b,item->right_offset,item->x, item->y, CHECKED_CHECK_BOX );
		else
			nm_rstring( b,item->right_offset,item->x, item->y, NORMAL_CHECK_BOX );
		break;
	case NM_TYPE_RADIO:
		nm_string( b, item->w, item->x, item->y, item->text );
		if (item->value)
			nm_rstring( b,item->right_offset, item->x, item->y, CHECKED_RADIO_BOX );
		else
			nm_rstring( b,item->right_offset, item->x, item->y, NORMAL_RADIO_BOX );
		break;
	case NM_TYPE_NUMBER:	{
		char text[10];
		if (item->value < item->min_value) item->value=item->min_value;
		if (item->value > item->max_value) item->value=item->max_value;
		nm_string( b, item->w, item->x, item->y, item->text );
		sprintf( text, "%d", item->value );
		nm_rstring( b,item->right_offset,item->x, item->y, text );
		}
		break;
	}

}

char *Newmenu_allowed_chars=NULL;
char *Newmenu_number_chars = "1900";

//returns true if char is allowed
static int char_allowed(char c)
{
	char *p = Newmenu_allowed_chars;

	if (!p)
		return 1;

	while (*p) {
		Assert(p[1]);

		if (c>=p[0] && c<=p[1])
			return 1;

		p += 2;
	}

	return 0;
}


void strip_end_whitespace( char * text )
{
	int i,l;
	l = strlen( text );
	for (i=l-1; i>=0; i-- )	{
//		if ( isspace(text[i]) )
		if ( (text[i] == ' ') || (text[i] == '\t'))
			text[i] = 0;
		else
			return;
	}
}

int newmenu_do( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) )
{
	return newmenu_do4( title, subtitle, nitems, item, subfunction, 0, NULL, -1, -1, 0 );
}

int newmenu_do1( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem )
{
	return newmenu_do4( title, subtitle, nitems, item, subfunction, citem, NULL, -1, -1, 0 );
}


int newmenu_do2( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename )
{
	return newmenu_do4( title, subtitle, nitems, item, subfunction, citem, filename, -1, -1, 0 );
}

int newmenu_do3( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height )
{
	return newmenu_do4( title, subtitle, nitems, item, subfunction, citem, filename, width, height, 0);
}

int newmenu_do4( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height, int close_box )
{
	int mx, my, x1, x2, y1, y2;
	int old_keyd_repeat, done;
	int  choice,old_choice,i,j,x,y,w,h,aw, tw, th, twidth,fm,right_offset;
	int k, nmenus, nothers, mouse_state, omouse_state;
	grs_canvas * save_canvas;
	grs_font * save_font;
	int string_width, string_height, average_width;
	int close_x, close_y, close_size;
	int ty;
	bkg bg;
	int all_text=0;		//set true if all text items
	int time_stopped=0;
	int r;

	//CDToDescentDir();								// safety check to get back where to need to

	if (nitems < 1 )
		return -1;

	set_screen_mode(SCREEN_MENU);

//NO_SOUND_PAUSE	if ( Function_mode == FMODE_GAME )	{
//NO_SOUND_PAUSE		digi_pause_all();
//NO_SOUND_PAUSE		sound_stopped = 1;
//NO_SOUND_PAUSE	}

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence)) )
	{
		time_stopped = 1;
		stop_time();
	}

	save_canvas = grd_curcanv;
	gr_set_current_canvas( NULL );
	save_font = grd_curcanv->cv_font;

	tw = th = 0;

	if ( title )	{
		grd_curcanv->cv_font = TITLE_FONT;
		gr_get_string_size(title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
	}
	if ( subtitle )	{
		grd_curcanv->cv_font = SUBTITLE_FONT;
		gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
		if (string_width > tw )
			tw = string_width;
		th += string_height;
	}

	th += 8;		//put some space between titles & body

	grd_curcanv->cv_font = NORMAL_FONT;

	w = aw = 0;
	h = th;
	nmenus = nothers = 0;

	// Find menu height & width (store in w,h)
	for (i=0; i<nitems; i++ )	{
		item[i].redraw=1;
		item[i].y = h;
		gr_get_string_size(item[i].text,&string_width,&string_height,&average_width );
		item[i].right_offset = 0;

		item[i].saved_text[0] = '\0';

		if ( item[i].type == NM_TYPE_SLIDER )	{
			int w1,h1,aw1;
			nothers++;
			sprintf( item[i].saved_text, "%s", SLIDER_LEFT );
			for (j=0; j<(item[i].max_value-item[i].min_value+1); j++ )	{
				sprintf( item[i].saved_text, "%s%s", item[i].saved_text,SLIDER_MIDDLE );
			}
			sprintf( item[i].saved_text, "%s%s", item[i].saved_text,SLIDER_RIGHT );
			gr_get_string_size(item[i].saved_text,&w1,&h1,&aw1 );
			string_width += w1 + aw;
		}

		if ( item[i].type == NM_TYPE_MENU )	{
			nmenus++;
		}

		if ( item[i].type == NM_TYPE_CHECK )	{
			int w1,h1,aw1;
			nothers++;
			gr_get_string_size(NORMAL_CHECK_BOX, &w1, &h1, &aw1  );
			item[i].right_offset = w1;
			gr_get_string_size(CHECKED_CHECK_BOX, &w1, &h1, &aw1  );
			if (w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}

		if (item[i].type == NM_TYPE_RADIO ) {
			int w1,h1,aw1;
			nothers++;
			gr_get_string_size(NORMAL_RADIO_BOX, &w1, &h1, &aw1  );
			item[i].right_offset = w1;
			gr_get_string_size(CHECKED_RADIO_BOX, &w1, &h1, &aw1  );
			if (w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}

		if  (item[i].type==NM_TYPE_NUMBER )	{
			int w1,h1,aw1;
			char test_text[20];
			nothers++;
			sprintf( test_text, "%d", item[i].max_value );
			gr_get_string_size( test_text, &w1, &h1, &aw1 );
			item[i].right_offset = w1;
			sprintf( test_text, "%d", item[i].min_value );
			gr_get_string_size( test_text, &w1, &h1, &aw1 );
			if ( w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}

		if ( item[i].type == NM_TYPE_INPUT )	{
			Assert( strlen(item[i].text) < NM_MAX_TEXT_LEN );
			strcpy(item[i].saved_text, item[i].text );
			nothers++;
			string_width = item[i].text_len*grd_curcanv->cv_font->ft_w+item[i].text_len;
			if ( string_width > MAX_TEXT_WIDTH )
				string_width = MAX_TEXT_WIDTH;
			item[i].value = -1;

			if (Newmenu_allowed_chars)
				item[i].next_key = Newmenu_allowed_chars[0];
			else if (item[i].min_value >=0)
				item[i].next_key = item[i].min_value;
			else
				item[i].next_key = 'A';
		}

		if ( item[i].type == NM_TYPE_INPUT_MENU )	{
			Assert( strlen(item[i].text) < NM_MAX_TEXT_LEN );
			strcpy(item[i].saved_text, item[i].text );
			nmenus++;
			string_width = item[i].text_len*grd_curcanv->cv_font->ft_w+item[i].text_len;
			item[i].value = -1;
			item[i].next_key = (Newmenu_allowed_chars==NULL) ? 'A' : Newmenu_allowed_chars[0];
			item[i].group = 0;
		}

		item[i].w = string_width;
		item[i].h = string_height;

		if ( string_width > w )
			w = string_width;		// Save maximum width
		if ( average_width > aw )
			aw = average_width;
		h += string_height+1;		// Find the height of all strings
	}

	right_offset=0;

	if ( width > -1 )
		w = width;

	if ( height > -1 )
		h = height;

	for (i=0; i<nitems; i++ )	{
		item[i].w = w;
		if (item[i].right_offset > right_offset )
			right_offset = item[i].right_offset;
	}
	if (right_offset > 0 )
		right_offset += 3;

	//mprintf( 0, "Right offset = %d\n", right_offset );

	//gr_get_string_size("",&string_width,&string_height,&average_width );

	w += right_offset;

	twidth = 0;
	if ( tw > w )	{
		twidth = ( tw - w )/2;
		w = tw;
	}

	// Find min point of menu border
//	x = (grd_curscreen->sc_w-w)/2;
//	y = (grd_curscreen->sc_h-h)/2;

	w += 30;
	h += 30;

	if ((r = (w & 0x7)))
		w += (8-r);			// align width to 8 byte boundry

	if (w > grd_curcanv->cv_bitmap.bm_w) w = grd_curcanv->cv_bitmap.bm_w;
	if (h > grd_curcanv->cv_bitmap.bm_h) h = grd_curcanv->cv_bitmap.bm_h;

	x = (grd_curcanv->cv_bitmap.bm_w-w)/2;
	if ((r = (x & 0x7)))
		x -= r;				// align x to 8 byte boundry
	y = (grd_curcanv->cv_bitmap.bm_h-h)/2;

	if ( x < 0 ) x = 0;
	if ( y < 0 ) y = 0;

	if ( filename != NULL )	{
		nm_draw_background1( filename );
	}

	// Save the background of the display
	bg.menu_canvas = gr_create_sub_canvas( &grd_curscreen->sc_canvas, x, y, w, h );
	gr_set_current_canvas( bg.menu_canvas );

	if ( filename == NULL )	{
		// Save the background under the menu...
		bg.saved = gr_create_bitmap( w, h );
		Assert( bg.saved != NULL );
		gr_bm_bitblt(w, h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.saved );
		bg.background = gr_create_sub_bitmap(&nm_background,0,0,w,h);
		gr_set_current_canvas( NULL );
		nm_draw_background(x,y,x+w-1,y+h-1);
		gr_set_current_canvas( bg.menu_canvas );
	} else {
		bg.saved = NULL;
		bg.background = gr_create_bitmap( w, h );
		Assert( bg.background != NULL );
		gr_bm_bitblt(w, h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.background );
	}

// ty = 15 + (yborder/4);

	ty = 15;

// draw a close box on the menu
	if (close_box) {
		close_x = close_y = 8;
		close_size = 10;
		gr_setcolor( BM_XRGB(0, 0, 0) );
		gr_rect(close_x, close_y, close_x + close_size, close_y + close_size);
		gr_setcolor( BM_XRGB(21, 21, 21) );
		gr_rect( close_x + 2, close_y + 2, close_x + close_size - 2, close_y + close_size -2 );
	}

	if ( title )	{
		grd_curcanv->cv_font = TITLE_FONT;
		gr_set_fontcolor( GR_GETCOLOR(31,31,31), -1 );
		gr_get_string_size(title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
		gr_printf( 0x8000, ty, title );
		ty += th;
	}

	if ( subtitle )	{
		grd_curcanv->cv_font = SUBTITLE_FONT;
		gr_set_fontcolor( GR_GETCOLOR(21,21,21), -1 );
		gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
		gr_printf( 0x8000, ty, subtitle );
		ty += th;
	}

	grd_curcanv->cv_font = NORMAL_FONT;

	// Update all item's x & y values.
	for (i=0; i<nitems; i++ )	{
		item[i].x = 15 + twidth + right_offset;
		item[i].y += 15;
		if ( item[i].type==NM_TYPE_RADIO )	{
			fm = -1;	// find first marked one
			for ( j=0; j<nitems; j++ )	{
				if ( item[j].type==NM_TYPE_RADIO && item[j].group==item[i].group )	{
					if (fm==-1 && item[j].value)
						fm = j;
					item[j].value = 0;
				}
			}
			if ( fm>=0 )
				item[fm].value=1;
			else
				item[i].value=1;
		}
	}

	old_keyd_repeat = keyd_repeat;
	keyd_repeat = 1;

	if (citem==-1)	{
		choice = -1;
	} else {
		if (citem < 0 ) citem = 0;
		if (citem > nitems-1 ) citem = nitems-1;
		choice = citem;

		while ( item[choice].type==NM_TYPE_TEXT )	{
			choice++;
			if (choice >= nitems ) {
				choice=0;
			}
			if (choice == citem ) {
				choice=0;
				all_text=1;
				break;
			}
		}
	}
	done = 0;

	// Clear mouse, joystick to clear button presses.
	game_flush_inputs();
	show_cursor();

	mouse_state = omouse_state = 0;

	while(!done)	{
		//network_listen();

#ifndef MAC_SHAREWARE
		//redbook_restart_track();		// restart any music that should be looping
#endif

// read mouse button every 1/10 second
		omouse_state = mouse_state;
		mouse_state = mouse_button_state(MB_LEFT);

		k = key_inkey();

		if (subfunction)
			(*subfunction)(nitems,item,&k,choice);

		if (!time_stopped)	{
			// Save current menu box
			#ifdef NETWORK
			if (multi_menu_poll() == -1)
				k = -2;
			#endif
		}

		if ( k<-1 ) {
			choice = k;
			k = -1;
			done = 1;
		}

		//for (i=0; i<2; i++)
		//	if (mouse_button_down_count(i)>0) done=1;

//		if ( (nmenus<2) && (k>0) && (nothers==0) )
//			done=1;

		old_choice = choice;

		switch( k )	{
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			//if (all_text) break;
			if (choice>-1 && (item[choice].type==NM_TYPE_INPUT || item[choice].type==NM_TYPE_TEXT || ((item[choice].type==NM_TYPE_INPUT_MENU) && item[choice].group==1)))
				break;
			do {
				choice--;
				if (choice >= nitems ) choice=0;
				if (choice < 0 ) choice=nitems-1;
			} while ( item[choice].type==NM_TYPE_TEXT );
			if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))
				item[choice].value = -1;
			if ((old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
				item[old_choice].group=0;
				strcpy(item[old_choice].text, item[old_choice].saved_text );
				item[old_choice].value = -1;
			}
			if (old_choice>-1)
				item[old_choice].redraw = 1;
			item[choice].redraw=1;
			break;
		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
			//if (all_text) break;
			if (choice>-1 && (item[choice].type==NM_TYPE_INPUT || item[choice].type==NM_TYPE_TEXT || ((item[choice].type==NM_TYPE_INPUT_MENU) && item[choice].group==1)))
				break;
			do {
				choice++;
				if (choice < 0 ) choice=nitems-1;
				if (choice >= nitems ) choice=0;
			} while ( item[choice].type==NM_TYPE_TEXT );
			if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))
				item[choice].value = -1;
			if ( (old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
				item[old_choice].group=0;
				strcpy(item[old_choice].text, item[old_choice].saved_text );
				item[old_choice].value = -1;
			}
			if (old_choice>-1)
				item[old_choice].redraw=1;
			item[choice].redraw=1;
			break;
		case KEY_SPACEBAR:
			if ( choice > -1 )	{
				switch( item[choice].type )	{
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if ( item[choice].value )
						item[choice].value = 0;
					else
						item[choice].value = 1;
					item[choice].redraw=1;
					break;
				case NM_TYPE_RADIO:
					for (i=0; i<nitems; i++ )	{
						if ((i!=choice) && (item[i].type==NM_TYPE_RADIO) && (item[i].group==item[choice].group) && (item[i].value) )	{
							item[i].value = 0;
							item[i].redraw = 1;
						}
					}
					item[choice].value = 1;
					item[choice].redraw = 1;
					break;
				}
			}
			break;

		case KEY_ENTER:
		case KEY_PADENTER:
			if ( (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==0))	{
				item[choice].group = 1;
				item[choice].redraw = 1;
//				if ( !strnicmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
				if ( !strnicmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
					item[choice].text[0] = 0;
					item[choice].value = -1;
				} else {
					strip_end_whitespace(item[choice].text);
				}
			} else
				done = 1;
			break;

		case KEY_ESC:
			if ( (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==1))	{
				item[choice].group=0;
				strcpy(item[choice].text, item[choice].saved_text );
				item[choice].redraw=1;
				item[choice].value = -1;
			} else {
				done = 1;
				choice = -1;
			}
			break;

		case KEY_PRINT_SCREEN:
		case KEY_COMMAND+KEY_SHIFTED+KEY_3:
			hide_cursor();
			save_screen_shot(0);
			show_cursor();
			break;

// CD controls follow here

#ifndef MAC_SHAREWARE
#if 0
		case KEY_COMMAND+KEY_E:
			redbook_eject_disk();
			break;
		case KEY_COMMAND+KEY_RIGHT:
			redbook_next_track();
			break;
		case KEY_COMMAND+KEY_LEFT:
			redbook_previous_track();
			break;
		case KEY_COMMAND+KEY_UP:
			redbook_start_play();
			break;
		case KEY_COMMAND+KEY_DOWN:
			stop_redbook();
			break;
		case KEY_COMMAND+KEY_M:
			redbook_mount_disk();
			break;
#endif
#endif

		case KEY_COMMAND+KEY_Q:
			if ( !(Game_mode & GM_MULTI) )
				do_appl_quit();
			show_cursor();
			break;

		#ifndef NDEBUG
		case KEY_BACKSP:
			if ( (choice>-1) && (item[choice].type!=NM_TYPE_INPUT)&&(item[choice].type!=NM_TYPE_INPUT_MENU))
				Int3();
			break;
		#endif

		}

		mouse_get_pos(&mx, &my);
		// button went down
		if ( !done && mouse_state && !omouse_state && !all_text ) {
#if 0
			for (i=0; i<nitems; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x - item[i].right_offset - 6;
				x2 = x1 + item[i].w;
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)))
				{
					choice = i;
#endif
					switch( item[choice].type )	{
					case NM_TYPE_CHECK:
						if ( item[choice].value )
							item[choice].value = 0;
						else
							item[choice].value = 1;
						item[choice].redraw=1;
						break;
					case NM_TYPE_RADIO:
						for (i=0; i<nitems; i++ )	{
							if ((i!=choice) && (item[i].type==NM_TYPE_RADIO) && (item[i].group==item[choice].group) && (item[i].value) )	{
								item[i].value = 0;
								item[i].redraw = 1;
							}
						}
						item[choice].value = 1;
						item[choice].redraw = 1;
						break;
					}
#if 0
					item[old_choice].redraw=1;
					break;
				}
			}
#endif
		}

		// mouse button pushed on an information screen
		if (mouse_state && all_text)
			done = 1;

		// mouse button released on close box
		if ( !done && !mouse_state && omouse_state && close_box ) {
			x1 = grd_curcanv->cv_bitmap.bm_x + close_x + 2;
			x2 = x1 + close_size - 2;
			y1 = grd_curcanv->cv_bitmap.bm_y + close_y + 2;
			y2 = y1 + close_size - 2;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				choice = -1;
				done = 1;
			}
		}

		// handle pointer relative events
		if ( !done && !all_text ) {
			for (i=0; i<nitems; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x - item[i].right_offset - 6;
				x2 = x1 + item[i].w;
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && (item[i].type != NM_TYPE_TEXT) ) {
					choice = i;
					if ((old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice)) {
						if (!mouse_state) {
							choice = old_choice;
							continue;
						}
						fprintf(stderr, "Setting group to 0\n");
						item[old_choice].group=0;
						strcpy(item[old_choice].text, item[old_choice].saved_text );
						item[old_choice].value = -1;
					}
					if ( item[choice].type == NM_TYPE_SLIDER && mouse_state) {
						char slider_text[NM_MAX_TEXT_LEN+1], *p, *s1;
						int slider_width, height, aw, sleft_width, sright_width, smiddle_width;

						strcpy(slider_text, item[choice].saved_text);
						p = strchr(slider_text, '\t');
						if (p) {
							*p = '\0';
							s1 = p+1;
						}
						if (p) {
							gr_get_string_size(s1, &slider_width, &height, &aw);
							gr_get_string_size(SLIDER_LEFT, &sleft_width, &height, &aw);
							gr_get_string_size(SLIDER_RIGHT, &sright_width, &height, &aw);
							gr_get_string_size(SLIDER_MIDDLE, &smiddle_width, &height, &aw);

							x1 = grd_curcanv->cv_bitmap.bm_x + item[choice].x + item[choice].w - slider_width;
							x2 = x1 + slider_width + sright_width;
							if ( (mx > x1) && (mx < (x1 + sleft_width)) && (item[choice].value != item[choice].min_value) ) {
								item[choice].value = item[choice].min_value;
								item[choice].redraw = 2;
							} else if ( (mx < x2) && (mx > (x2 - sright_width)) && (item[choice].value != item[choice].max_value) ) {
								item[choice].value = item[choice].max_value;
								item[choice].redraw = 2;
							} else if ( (mx > (x1 + sleft_width)) && (mx < (x2 - sright_width)) ) {
								int num_values, value_width, new_value;

								num_values = item[choice].max_value - item[choice].min_value + 1;
								value_width = (slider_width - sleft_width - sright_width) / num_values;
								new_value = (mx - x1 - sleft_width) / value_width;
								if ( item[choice].value != new_value ) {
									item[choice].value = new_value;
									item[choice].redraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (choice == old_choice)
						break;
					if (item[choice].type==NM_TYPE_INPUT_MENU && !mouse_state) {
						choice = old_choice;
						continue;
					}
					if (item[choice].type==NM_TYPE_INPUT)
						item[choice].value = -1;
					if (old_choice>-1)
						item[old_choice].redraw = 1;
					item[choice].redraw=1;
					break;
				}
			}
		}

		// mouse button released over a menu selection
		if ( !done && !mouse_state && omouse_state && !all_text && (choice != -1) ) {
#if 0
			x1 = grd_curcanv->cv_bitmap.bm_x + item[choice].x;
			x2 = x1 + item[choice].w;
			y1 = grd_curcanv->cv_bitmap.bm_y + item[choice].y;
			y2 = y1 + item[choice].h;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)))
#endif
			if (item[choice].type==NM_TYPE_INPUT) {
				// move down
				do {
					choice++;
					if (choice < 0 ) choice=nitems-1;
					if (choice >= nitems ) choice=0;
				} while ( item[choice].type==NM_TYPE_TEXT );
				if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))
					item[choice].value = -1;
				if ( (old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
					item[old_choice].group=0;
					strcpy(item[old_choice].text, item[old_choice].saved_text );
					item[old_choice].value = -1;
				}
				if (old_choice>-1)
					item[old_choice].redraw=1;
				item[choice].redraw=1;
			}
			if (item[choice].type==NM_TYPE_MENU || (item[choice].type==NM_TYPE_INPUT && old_choice==choice))
				done = 1;
		}

		// mouse button released when waiting for text input
		if (!done && !mouse_state && omouse_state && (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU)) {
			if (item[choice].group==0) {
				item[choice].group = 1;
				item[choice].redraw = 1;
				if ( !strnicmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
					item[choice].text[0] = 0;
					item[choice].value = -1;
				} else {
					strip_end_whitespace(item[choice].text);
				}
			}
			else if (choice == old_choice) {
				done = 1;
			}
		}

		if ( choice > -1 )	{
			unsigned char ascii;

			if ( ((item[choice].type==NM_TYPE_INPUT)||((item[choice].type==NM_TYPE_INPUT_MENU)&&(item[choice].group==1)) )&& (old_choice==choice) )	{
				if ( k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4 )	{
					if (item[choice].value==-1) item[choice].value = strlen(item[choice].text);
					if (item[choice].value > 0)
						item[choice].value--;
					item[choice].text[item[choice].value] = 0;
					item[choice].redraw = 1;
				} else if (k==KEY_RIGHT || k==KEY_PAD6) {
					if (item[choice].value==-1) item[choice].value = strlen(item[choice].text);
					if (item[choice].value < item[choice].text_len) {
						item[choice].text[item[choice].value++] = item[choice].next_key;
						item[choice].text[item[choice].value] = '\0';
						item[choice].redraw = 1;
					}
				} else if (k==KEY_UP || k==KEY_PAD8) {
					ascii = item[choice].next_key;
					do {
						ascii--;
					} while (!char_allowed(ascii));
					if (Newmenu_allowed_chars == NULL) {
						if (item[choice].min_value>=0 && ascii<item[choice].min_value)
							ascii = item[choice].min_value;
						else if (item[choice].max_value>0 && ascii>item[choice].max_value)
							ascii = item[choice].max_value;
					}
					item[choice].next_key = ascii;
					item[choice].redraw = 1;
				} else if (k==KEY_DOWN || k==KEY_PAD2) {
					ascii = item[choice].next_key;
					do {
						ascii++;
					} while (!char_allowed(ascii));
					if (Newmenu_allowed_chars == NULL) {
						if (item[choice].min_value>=0 && ascii<item[choice].min_value)
							ascii = item[choice].min_value;
						else if (item[choice].max_value>0 && ascii>item[choice].max_value)
							ascii = item[choice].max_value;
					}
					item[choice].next_key = ascii;
					item[choice].redraw = 1;
				} else {
					ascii = key_to_ascii(k);
					if ((ascii < 255 ) && (item[choice].value < item[choice].text_len ))
					{
						int allowed;

						if (item[choice].value==-1) {
							item[choice].value = 0;
						}

						allowed = char_allowed(ascii);

						if (!allowed && ascii==' ' && char_allowed('_')) {
							ascii = '_';
							allowed=1;
						}

						if (allowed) {
							item[choice].text[item[choice].value++] = ascii;
							item[choice].text[item[choice].value] = 0;
							item[choice].redraw=1;
						}
					}
				}
			} else if ((item[choice].type!=NM_TYPE_INPUT) && (item[choice].type!=NM_TYPE_INPUT_MENU) ) {
				ascii = key_to_ascii(k);
				if (ascii < 255 ) {
					int choice1 = choice;
					ascii = toupper(ascii);
					do {
						int i,ch;
						choice1++;
						if (choice1 >= nitems ) choice1=0;
						for (i=0;(ch=item[choice1].text[i])!=0 && ch==' ';i++);
						if ( ( (item[choice1].type==NM_TYPE_MENU) ||
								 (item[choice1].type==NM_TYPE_CHECK) ||
								 (item[choice1].type==NM_TYPE_RADIO) ||
								 (item[choice1].type==NM_TYPE_NUMBER) ||
								 (item[choice1].type==NM_TYPE_SLIDER) )
								&& (ascii==toupper(ch)) )	{
							k = 0;
							choice = choice1;
							if (old_choice>-1)
								item[old_choice].redraw=1;
							item[choice].redraw=1;
						}
					} while (choice1 != choice );
				}
			}

			if ( (item[choice].type==NM_TYPE_NUMBER) || (item[choice].type==NM_TYPE_SLIDER)) 	{
				int ov=item[choice].value;
				switch( k ) {
				case KEY_PAD4:
			  	case KEY_LEFT:
			  	case KEY_MINUS:
				case KEY_MINUS+KEY_SHIFTED:
				case KEY_PADMINUS:
					item[choice].value -= 1;
					break;
			  	case KEY_RIGHT:
				case KEY_PAD6:
			  	case KEY_EQUAL:
				case KEY_EQUAL+KEY_SHIFTED:
				case KEY_PADPLUS:
					item[choice].value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					item[choice].value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					item[choice].value -= 10;
					break;
				}
				if (ov!=item[choice].value)
					item[choice].redraw=1;
			}

		}

		gr_set_current_canvas(bg.menu_canvas);
		// Redraw everything...
		for (i=0; i<nitems; i++ )	{
			if (item[i].redraw)	{
				hide_cursor();
				draw_item( &bg, &item[i], (i==choice && !all_text) );
				show_cursor();
				item[i].redraw=0;
			}
			else if (i==choice && (item[i].type==NM_TYPE_INPUT || (item[i].type==NM_TYPE_INPUT_MENU && item[i].group)))
				update_cursor( &item[i]);
		}
		bitblt_to_screen();
		if ( gr_palette_faded_out )	{
			gr_palette_fade_in( gr_palette, 32, 0 );
		}
	}
	hide_cursor();

	// Restore everything...
	gr_set_current_canvas(bg.menu_canvas);
	if ( filename == NULL )	{
		// Save the background under the menu...
		gr_bitmap(0, 0, bg.saved);
		gr_free_bitmap(bg.saved);
		myfree( bg.background );
	} else {
		gr_bitmap(0, 0, bg.background);
		gr_free_bitmap(bg.background);
	}

	gr_free_sub_canvas( bg.menu_canvas );

	gr_set_current_canvas( NULL );
	grd_curcanv->cv_font	= save_font;
	gr_set_current_canvas( save_canvas );
	keyd_repeat = old_keyd_repeat;

	game_flush_inputs();

	if (time_stopped)
		start_time();

//NO_SOUND_PAUSE	if ( sound_stopped )
//NO_SOUND_PAUSE		digi_resume_all();

	return choice;

}


int nm_messagebox1( char *title, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int nchoices, ... )
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices );

	Assert( nchoices <= 5 );

	for (i=0; i<nchoices; i++ )	{
		s = va_arg( args, char * );
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	format = va_arg( args, char * );
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE);

	return newmenu_do( title, nm_text, nchoices, nm_message_items, subfunction );
}

int nm_messagebox( char *title, int nchoices, ... )
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices );

	Assert( nchoices <= 5 );

	for (i=0; i<nchoices; i++ )	{
		s = va_arg( args, char * );
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	format = va_arg( args, char * );
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE );

	return newmenu_do( title, nm_text, nchoices, nm_message_items, NULL );
}




void newmenu_file_sort( int n, char *list )
{
	int i, j, incr;
	char t[14];

	incr = n / 2;
	while( incr > 0 )		{
		for (i=incr; i<n; i++ )		{
			j = i - incr;
			while (j>=0 )			{
				if (strnicmp(&list[j*14], &list[(j+incr)*14], 12) > 0)				{
					memcpy( t, &list[j*14], 13 );
					memcpy( &list[j*14], &list[(j+incr)*14], 13 );
					memcpy( &list[(j+incr)*14], t, 13 );
					j -= incr;
				}
				else
					break;
			}
		}
		incr = incr / 2;
	}
}

void delete_player_saved_games(char * name)
{
	int i;
	char filename[64];

	for (i=0;i<10; i++)	{
		sprintf( filename, ":Players:%s.sg%d", name, i );
		remove(filename);
	}
}

#define MAX_FILES 200

int MakeNewPlayerFile(int allow_abort);

int newmenu_get_filename( char * title, char * filespec, char * filename, int allow_abort_flag )
{
	int i;
	DIR* dir;
	struct dirent* entry;
	int mx, my, x1, x2, y1, y2, mouse_state, omouse_state, r;
	int NumFiles=0, key,done, citem, ocitem;
	char * filenames = NULL;
	const int NumFiles_displayed = 8;
	int first_item = -1, ofirst_item;
	int old_keyd_repeat = keyd_repeat;
	int player_mode=0;
	int demo_mode=0;
	int demos_deleted=0;
	int initialized = 0;
	int exit_value = 0;
	int w_x=0, w_y=0, w_w, w_h;
	int close_x, close_y, close_size, close_box;

	close_box = allow_abort_flag;
	filenames = mymalloc( MAX_FILES * 14 );
	if (filenames==NULL) return 0;

ReadFileNames:
	citem = 0;
	first_item = -1;
	keyd_repeat = 1;

	//CDToDescentDir();								// safety check to get back where to need to
	if (!stricmp( filespec, "Players" ))
		player_mode = 1;
	else if (!stricmp( filespec, "Demos" ))
		demo_mode = 1;

	done = 0;
	NumFiles=0;

#ifndef APPLE_OEM		// no new pilots for special apple oem version
	if (player_mode)	{
		strncpy( &filenames[NumFiles*14], TXT_CREATE_NEW, 13 );
		NumFiles++;
	}
#endif

	dir = opendir(".");
	while ((entry = readdir(dir))) {
		char *t = strchr(entry->d_name, '.');
		if (t==NULL)
			continue;
		if (player_mode && stricmp(t, ".plr"))
			continue;
		else if (demo_mode && stricmp(t, ".dem"))
			continue;
		if (NumFiles<MAX_FILES)	{
			strncpy( &filenames[NumFiles*14], entry->d_name, 13 );
			if ( player_mode )	{
				char * p;
				p = strchr(&filenames[NumFiles*14],'.');
				if (p) *p = '\0';
			}
			NumFiles++;
		} else {
			break;
		}
	}
	closedir(dir);

#if 0
#ifdef SATURN
	// Seach CD for files if demo_mode and cd_mode
	if ( strlen(destsat_cdpath) && demo_mode )	{
		char temp_spec[128];
		strcpy( temp_spec, destsat_cdpath );
		strcat( temp_spec, filespec );

		if( !_dos_findfirst( temp_spec, 0, &find ) )	{
			do	{
				if (NumFiles<MAX_FILES)	{

					for (i=0; i<NumFiles; i++ )	{
						if (!stricmp( &filenames[i*14], find.name ))
							break;
					}
					if ( i < NumFiles ) continue;		// Don't use same demo twice!

					strncpy( &filenames[NumFiles*14], find.name, 13 );
					if ( player_mode )	{
						char * p;
						p = strchr(&filenames[NumFiles*14],'.');
						if (p) *p = '\0';
					}
					NumFiles++;
				} else {
					break;
				}
			} while( !_dos_findnext( &find ) );
		}
	}
#endif
#endif // #if 0

	if ( (NumFiles < 1) && demos_deleted )	{
		exit_value = 0;
		goto ExitFileMenu;
	}
	if ( (NumFiles < 1) && demo_mode ) {
		nm_messagebox( NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
		exit_value = 0;
		goto ExitFileMenu;
	}

	#ifndef APPLE_OEM			// really for testing, but this is true since we cannot add players
	if ( (NumFiles < 2) && player_mode ) {
		citem = 0;
		goto ExitFileMenuEarly;
	}
	#endif

	if ( NumFiles<1 )	{
		nm_messagebox( NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, filespec, TXT_WERE_FOUND);
		exit_value = 0;
		goto ExitFileMenu;
	}

	if (!initialized) {
		set_screen_mode(SCREEN_MENU);
		gr_set_current_canvas(NULL);

		w_w = 230 - 90 + 1 + 30;
		w_h = 170 - 30 + 1 + 30;

		if ( w_w > 320 ) w_w = 320;
		if ( w_h > 200 ) w_h = 200;

		w_x = (grd_curcanv->cv_bitmap.bm_w-w_w)/2;
		if ((r = w_x & 0x7)) {
			for (i = 0; i < r; i++)
				w_x--;
		}
		w_y = (grd_curcanv->cv_bitmap.bm_h-w_h)/2;

		if ( w_x < 0 ) w_x = 0;
		if ( w_y < 0 ) w_y = 0;

		gr_bm_bitblt(grd_curscreen->sc_w, grd_curscreen->sc_h, 0, 0, 0, 0, &(grd_curcanv->cv_bitmap), &(VR_offscreen_buffer->cv_bitmap) );
		nm_draw_background( w_x,w_y,w_x+w_w-1,w_y+w_h-1 );
		bitblt_to_screen();

		grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];
		gr_string( 0x8000, w_y+10, title );

		if (close_box) {
			close_x = close_y = 8;
			close_size = 10;
			gr_setcolor( BM_XRGB(0, 0, 0) );
			gr_rect(w_x+close_x, w_y+close_y, w_x+close_x + close_size, w_y+close_y + close_size);
			gr_setcolor( BM_XRGB(21, 21, 21) );
			gr_rect( w_x+close_x + 2, w_y+close_y + 2, w_x+close_x + close_size - 2, w_y+close_y + close_size -2 );
		}

		initialized = 1;
	}

	if ( !player_mode )	{
		newmenu_file_sort( NumFiles, filenames );
	} else {
		#ifndef APPLE_OEM
		newmenu_file_sort( NumFiles-1, &filenames[14] );		// Don't sort first one!
		#else
		newmenu_file_sort( NumFiles, &filenames[14] );		// sort the first one for apple oem version since you can't add pilots
		#endif
		for ( i=0; i<NumFiles; i++ )	{
			if (!stricmp(Players[Player_num].callsign, &filenames[i*14]) )
				citem = i;
		}
	}

	omouse_state = mouse_state = 0;
	show_cursor();

	while(!done)	{
		ocitem = citem;
		ofirst_item = first_item;
		omouse_state = mouse_state;
		mouse_state = mouse_button_state(0);
		key = key_inkey();

		// wiimote minus
		if (joy_get_button_down_cnt(2) > 0)
			key = KEY_CTRLED+KEY_D;

		switch(key)	{
		case KEY_COMMAND+KEY_SHIFTED+KEY_3:
		case KEY_PRINT_SCREEN:
			hide_cursor();
			save_screen_shot(0);
			show_cursor();
			break;

		#ifndef APPLE_OEM			// not allowed to delete players in Apple OEM version
		case KEY_CTRLED+KEY_D:
			if ( ((player_mode)&&(citem>0)) || ((demo_mode)&&(citem>=0)) )	{
				int x = 1;
				hide_cursor();
				if (player_mode)
					x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, &filenames[citem*14]+((player_mode && filenames[citem*14]=='$')?1:0) );
				else if (demo_mode)
					x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, &filenames[citem*14]+((demo_mode && filenames[citem*14]=='$')?1:0) );
				show_cursor();
 				if (x==0)	{
					char long_name[64];
					char * p;
					int ret;
					long_name[0] = '\0';
					p = &filenames[(citem*14)+strlen(&filenames[citem*14])];
					//strcpy(long_name, "/Descent/");
					if (player_mode) {
						*p = '.';
					}
					strcpy(long_name, &filenames[citem*14]);
					ret = remove(long_name);
					if (player_mode)
						*p = 0;

					if ((!ret) && player_mode)	{
						delete_player_saved_games( &filenames[citem*14] );
					}

					if (ret) {
						if (player_mode)
							nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, &filenames[citem*14]+((player_mode && filenames[citem*14]=='$')?1:0) );
						else if (demo_mode)
							nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, &filenames[citem*14]+((demo_mode && filenames[citem*14]=='$')?1:0) );
					} else if (demo_mode)
						demos_deleted = 1;
					first_item = -1;
					goto ReadFileNames;
				}
			}
			break;
		#endif				// ifndef APPLE_OEM

#ifndef NDEBUG
		case KEY_BACKSP:
			Int3();
			break;
#endif
		case KEY_HOME:
		case KEY_PAD7:
			citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			citem = NumFiles-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			citem--;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			citem++;
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			citem += NumFiles_displayed;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			citem -= NumFiles_displayed;
			break;
		case KEY_ESC:
			if (allow_abort_flag) {
				citem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;
		case KEY_COMMAND+KEY_Q:
			if ( !(Game_mode & GM_MULTI) )
				do_appl_quit();
			show_cursor();
			break;
		default:
			{
				int ascii = key_to_ascii(key);
				if ( ascii < 255 )	{
					int cc,cc1;
					cc=cc1=citem+1;
					if (cc1 < 0 )  cc1 = 0;
					if (cc1 >= NumFiles )  cc1 = 0;
					while(1) {
						if ( cc < 0 ) cc = 0;
						if ( cc >= NumFiles ) cc = 0;
						if ( citem == cc ) break;

						if ( toupper((int)filenames[cc*14]) == toupper(ascii) )	{
							citem = cc;
							break;
						}
						cc++;
					}
				}
			}
		}
		if ( done ) break;

		if (citem<0)
			citem=0;

		if (citem>=NumFiles)
			citem = NumFiles-1;

		if (citem< first_item)
			first_item = citem;

		if (citem>=( first_item+NumFiles_displayed))
			first_item = citem-NumFiles_displayed+1;

		if (NumFiles <= NumFiles_displayed )
			 first_item = 0;

		if (first_item>NumFiles-NumFiles_displayed)
			first_item = NumFiles-NumFiles_displayed;
		if (first_item < 0 ) first_item = 0;

		mouse_get_pos(&mx, &my);
		for (i=first_item; i<first_item+NumFiles_displayed; i++ )	{
			x1 = 100;
			x2 = 220;
			y1 = (i-first_item)*12+w_y+45;
			y2 = y1+13;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				if (i == citem || i >= NumFiles)
					break;
				citem = i;
				break;
			}
		}

		if (!mouse_state && omouse_state) {
#if 0
			x1 = 100;
			x2 = 220;
			y1 = (citem-first_item)*12+w_y+45;
			y2 = y1+13;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) )
#endif
				done = 1;
		}

		if ( !mouse_state && omouse_state && close_box ) {
			x1 = w_x + close_x + 2;
			x2 = x1 + close_size - 2;
			y1 = w_y + close_y + 2;
			y2 = y1 + close_size - 2;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				citem = -1;
				done = 1;
			}
		}

		if (ofirst_item != first_item )	{
			gr_setcolor( BM_XRGB( 0,0,0)  );
			hide_cursor();
			for (i=first_item; i<first_item+NumFiles_displayed; i++ )	{
				int w, h, aw, y;
				y = (i-first_item)*12+w_y+45;
				if ( i >= NumFiles )	{
					gr_setcolor( BM_XRGB(0,0,0));
					gr_rect( 100, y-1, 220, y+11 );
				} else {
					if ( i == citem )
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
					else
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
					gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
					gr_rect( 100, y-1, 220, y+11 );
					gr_string( 105, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
				}
			}
			show_cursor();
		} else if ( citem != ocitem )	{
			int w, h, aw, y;

			hide_cursor();
			i = ocitem;
			if ( (i>=0) && (i<NumFiles) )	{
				y = (i-first_item)*12+w_y+45;
				if ( i == citem )
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
				gr_rect( 100, y-1, 220, y+11 );
				gr_string( 105, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
			}
			i = citem;
			if ( (i>=0) && (i<NumFiles) )	{
				y = (i-first_item)*12+w_y+45;
				if ( i == citem )
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
				gr_rect( 100, y-1, 220, y+11 );
				gr_string( 105, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
			}
			show_cursor();
		}
		bitblt_to_screen();
	}

ExitFileMenuEarly:
	hide_cursor();
	if ( citem > -1 )	{
		strncpy( filename, (&filenames[citem*14])+((player_mode && filenames[citem*14]=='$')?1:0), 13 );
		exit_value = 1;
	} else {
		exit_value = 0;
	}

ExitFileMenu:
	keyd_repeat = old_keyd_repeat;

	if ( initialized )	{
		gr_bm_bitblt(grd_curscreen->sc_w, grd_curscreen->sc_h, 0, 0, 0, 0, &(VR_offscreen_buffer->cv_bitmap), &(grd_curcanv->cv_bitmap) );
		bitblt_to_screen();
	}

	if ( filenames )
		myfree(filenames);

	return exit_value;

}


// Example listbox callback function...
// int lb_callback( int * citem, int *nitems, char * items[], int *keypress )
// {
// 	int i;
//
// 	if ( *keypress = KEY_CTRLED+KEY_D )	{
// 		if ( *nitems > 1 )	{
// 			unlink( items[*citem] );		// Delete the file
// 			for (i=*citem; i<*nitems-1; i++ )	{
// 				items[i] = items[i+1];
// 			}
// 			*nitems = *nitems - 1;
// 			free( items[*nitems] );
// 			items[*nitems] = NULL;
// 			return 1;	// redraw;
// 		}
//			*keypress = 0;
// 	}
// 	return 0;
// }

#define LB_ITEMS_ON_SCREEN 8

int newmenu_listbox( char * title, int nitems, char * items[], int allow_abort_flag, int (*listbox_callback)( int * citem, int *nitems, char * items[], int *keypress ) )
{
	return newmenu_listbox1( title, nitems, items, allow_abort_flag, 0, listbox_callback, 1 );
}

int newmenu_listbox1( char * title, int nitems, char * items[], int allow_abort_flag, int default_item, int (*listbox_callback)( int * citem, int *nitems, char * items[], int *keypress ), int close_box )
{
	int i;
	int mx, my, x1, x2, y1, y2;
	int done, ocitem,citem, ofirst_item, first_item, key, redraw;
	int old_keyd_repeat = keyd_repeat;
	int width, height, wx, wy, title_height;
	int mouse_state, omouse_state;
	int close_x, close_y, close_size;

	keyd_repeat = 1;

	set_screen_mode(SCREEN_MENU);
	gr_set_current_canvas(NULL);
	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	width = 0;
	for (i=0; i<nitems; i++ )	{
		int w, h, aw;
		gr_get_string_size( items[i], &w, &h, &aw );
		if ( w > width )
			width = w;
	}
	height = 12 * LB_ITEMS_ON_SCREEN;

	{
		int w, h, aw;
		gr_get_string_size( title, &w, &h, &aw );
		if ( w > width )
			width = w;
		title_height = h + 13;
	}

	width += 10;
	if ( width > grd_curcanv->cv_bitmap.bm_w - 30 )
		width = grd_curcanv->cv_bitmap.bm_w - 30;

	wx = (grd_curcanv->cv_bitmap.bm_w-width)/2;
	wy = (grd_curcanv->cv_bitmap.bm_h-(height+title_height))/2 + title_height;
	if ( wy < title_height )
		wy = title_height;

	gr_bm_bitblt(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h, 0, 0, 0, 0, &(grd_curcanv->cv_bitmap), &(VR_offscreen_buffer->cv_bitmap) );
	nm_draw_background( wx-15,wy-title_height-15,wx+width+15,wy+height+15 );

	gr_string( 0x8000, wy - title_height, title );

	if (close_box) {
		close_x = wx - 15 + 8;
		close_y = wy - title_height - 16 + 8;
		close_size = 10;
		gr_setcolor( BM_XRGB(0, 0, 0) );
		gr_rect(close_x, close_y, close_x + close_size, close_y + close_size);
		gr_setcolor( BM_XRGB(21, 21, 21) );
		gr_rect( close_x + 2, close_y + 2, close_x + close_size - 2, close_y + close_size - 2 );
	} else
		close_x = close_y = 0;

	done = 0;
	citem = default_item;
	if ( citem < 0 ) citem = 0;
	if ( citem >= nitems ) citem = 0;

	first_item = -1;

	omouse_state = mouse_state = 0;
	show_cursor();

	while(!done)	{
		ocitem = citem;
		ofirst_item = first_item;
		omouse_state = mouse_state;
		mouse_state = mouse_button_state(0);
		key = key_inkey();

		if ( listbox_callback )
			redraw = (*listbox_callback)(&citem, &nitems, items, &key );
		else
			redraw = 0;

		if ( key<-1 ) {
			citem = key;
			key = -1;
			done = 1;
		}

		switch(key)	{
		case KEY_PRINT_SCREEN:
		case KEY_COMMAND+KEY_SHIFTED+KEY_3:
			hide_cursor();
			save_screen_shot(0);
			show_cursor();
			break;
		case KEY_HOME:
		case KEY_PAD7:
			citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			citem = nitems-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			citem--;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			citem++;
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			citem += LB_ITEMS_ON_SCREEN;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			citem -= LB_ITEMS_ON_SCREEN;
			break;
		case KEY_ESC:
			if (allow_abort_flag) {
				citem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;

		case KEY_COMMAND+KEY_Q:
			if ( !(Game_mode & GM_MULTI) )
				do_appl_quit();
			show_cursor();
			break;
		default:
			if ( key > 0 )	{
				int ascii = key_to_ascii(key);
				if ( ascii < 255 )	{
					int cc,cc1;
					cc=cc1=citem+1;
					if (cc1 < 0 )  cc1 = 0;
					if (cc1 >= nitems )  cc1 = 0;
					while(1) {
						if ( cc < 0 ) cc = 0;
						if ( cc >= nitems ) cc = 0;
						if ( citem == cc ) break;

						if ( toupper((int)items[cc][0] ) == toupper(ascii) )	{
							citem = cc;
							break;
						}
						cc++;
					}
				}
			}
		}
		if ( done ) break;

		if (citem<0)
			citem=0;

		if (citem>=nitems)
			citem = nitems-1;

		if (citem< first_item)
			first_item = citem;

		if (citem>=( first_item+LB_ITEMS_ON_SCREEN))
			first_item = citem-LB_ITEMS_ON_SCREEN+1;

		if (nitems <= LB_ITEMS_ON_SCREEN )
			 first_item = 0;

		if (first_item>nitems-LB_ITEMS_ON_SCREEN)
			first_item = nitems-LB_ITEMS_ON_SCREEN;
		if (first_item < 0 ) first_item = 0;

		mouse_get_pos(&mx, &my);
		// move highlight
		for (i=first_item; i<first_item+LB_ITEMS_ON_SCREEN; i++ )	{
			x1 = wx;
			x2 = wx + width-1;
			y1 = (i-first_item)*12+wy;
			y2 = y1+13;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				if (i == citem)
					break;
				citem = i;
				break;
			}
		}

		// get clicked item
		if (!mouse_state && omouse_state) {
#if 0
			x1 = wx;
			x2 = wx + width-1;
			y1 = (citem-first_item)*12+wy;
			y2 = y1+13;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) )
#endif
				done = 1;
		}

		// close box clicked
		if ( !mouse_state && omouse_state && close_box ) {
			x1 = close_x + 2;
			x2 = x1 + close_size - 2;
			y1 = close_y + 2;
			y2 = y1 + close_size - 2;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				citem = -1;
				done = 1;
			}
		}

		if ( (ofirst_item != first_item) || redraw)	{
			hide_cursor();
			gr_setcolor( BM_XRGB( 0,0,0)  );
			for (i=first_item; i<first_item+LB_ITEMS_ON_SCREEN; i++ )	{
				int w, h, aw, y;
				y = (i-first_item)*12+wy;
				if ( i >= nitems )	{
					gr_setcolor( BM_XRGB(0,0,0));
					gr_rect( wx, y-1, wx+width-1, y+11 );
				} else {
					if ( i == citem )
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
					else
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
					gr_get_string_size(items[i], &w, &h, &aw  );
					gr_rect( wx, y-1, wx+width-1, y+11 );
					gr_string( wx+5, y, items[i]  );
				}
			}
			show_cursor();
		} else if ( citem != ocitem )	{
			int w, h, aw, y;

			hide_cursor();
			i = ocitem;
			if ( (i>=0) && (i<nitems) )	{
				y = (i-first_item)*12+wy;
				if ( i == citem )
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size(items[i], &w, &h, &aw  );
				gr_rect( wx, y-1, wx+width-1, y+11 );
				gr_string( wx+5, y, items[i]  );
			}
			i = citem;
			if ( (i>=0) && (i<nitems) )	{
				y = (i-first_item)*12+wy;
				if ( i == citem )
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size( items[i], &w, &h, &aw  );
				gr_rect( wx, y-1, wx+width-1, y+11 );
				gr_string( wx+5, y, items[i]  );
			}
			show_cursor();
		}
		bitblt_to_screen();
	}
	hide_cursor();
	keyd_repeat = old_keyd_repeat;

	gr_bm_bitblt(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h, 0, 0, 0, 0, &(VR_offscreen_buffer->cv_bitmap), &(grd_curcanv->cv_bitmap) );
	bitblt_to_screen();

	return citem;
}


// assume chdir'ed to this directory

int newmenu_filelist( char * title, char * filespec, char * filename )
{
	int i, NumFiles;
	char * Filenames[MAX_FILES];
	char FilenameText[MAX_FILES][14];
	DIR* dir;
	struct dirent* entry;

	NumFiles = 0;
	dir = opendir(filespec);
	while ((entry = readdir(dir))) {
		if (NumFiles<MAX_FILES)	{
			if (strlen(entry->d_name)<13) {
				strncpy( FilenameText[NumFiles], entry->d_name, 13 );
				Filenames[NumFiles] = FilenameText[NumFiles];
				NumFiles++;
			}
		} else {
			break;
		}
	}
	closedir(dir);
	i = newmenu_listbox( title, NumFiles, Filenames, 1, NULL );
	if ( i > -1 )	{
		strcpy( filename, Filenames[i] );
		return 1;
	}
	return 0;
}

