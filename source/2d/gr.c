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
#include <string.h>

#include <ogcsys.h>

#include "mem.h"
#include "gr.h"
#include "grdef.h"
#include "error.h"
#include "mono.h"
#include "palette.h"
#include "wiisys.h"

char gr_pal_default[768];

int gr_installed = 0;

int gr_show_screen_info = 0;

void gr_close()
{
	gr_set_current_canvas(NULL);
	gr_clear_canvas( BM_XRGB(0,0,0) );
	if (gr_installed==1)
	{
		gr_installed = 0;
		myfree(grd_curscreen);
	}
}

int gr_set_mode(int mode)
{
	int w, h;
	int x=0;
	int y=0;

	switch (mode) {
		case SM_320x200x8:
			w = 320;
			h = 200;
			break;

		case SM_320x200x8UL:
			w = 320;
			h = 200;
			y = 21;
			break;

		case SM_320x200x16:
			w = 320;
			h = 200;
			break;

		case SM_320x400U:
			w = 320;
			h = 400;
			break;

		case SM_ORIGINAL:
		case SM_640x480V:
			w = 640;
			h = 480;
			break;
		default:
			w=0;
			h=0;
			Error("Unknown graphics mode requested");
	}

	gr_palette_clear();
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
#ifdef __wii__
	// this is pixel aspect ratio, height vs. width (backwards)
	// smaller number makes things taller
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		grd_curscreen->sc_aspect = (F1_0*3)/4;
	else
#endif
		grd_curscreen->sc_aspect = F1_0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_x = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_y = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = BM_LINEAR;
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = MonitorRowBytes;
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = (ubyte *)(MonitorData + (MonitorRowBytes * y) + x);
	gr_set_current_canvas(NULL);

	set_win_size(x, y, w, h);
	//gr_enable_default_palette_loading();

	return 0;
}

extern void gr_build_mac_gamma(double correction);
extern double gamma_corrections[9];
extern ubyte gr_palette_gamma;

int gr_init(int mode)
{
	int retcode;

	// Only do this function once!
	if (gr_installed==1)
		return 1;

	// Save the current palette, and fade it out to black.

	MALLOC( grd_curscreen,grs_screen,1 );
	memset( grd_curscreen, 0, sizeof(grs_screen));

// initialize the macintosh window that we will use -- including picking the
// monitor

	if ((retcode=wiiwin_init()))
		return retcode;

	// Set the mode.
	if ((retcode=gr_set_mode(mode)))
		return retcode;

//JOHNgr_disable_default_palette_loading();

	gr_build_mac_gamma(gamma_corrections[gr_palette_gamma]);

	// Set all the screen, canvas, and bitmap variables that
	// aren't set by the gr_set_mode call:
	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	// Set flags indicating that this is installed.
	gr_installed = 1;
	atexit(gr_close);

	return 0;
}
