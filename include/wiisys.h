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

#ifndef _WII_SYS
#define _WII_SYS


#include "gr.h"

// global defintions for the window and gworld on which the game is currently
// being played on and drawn to

#define Compatibility_mode 0

extern const int MonitorRowBytes;   // row bytes for monitor being used
extern const ubyte *MonitorData;    // pointer to screen data for monitor;
extern ubyte have_virtual_memory;   // is virtual memory turned on??
extern ubyte use_sounds;            // what type of sound usage
extern ubyte use_alt_textures;      // what type of texture usage

extern int wii_init();
extern int wiiwin_init();
extern void set_win_size(int x, int y, int w, int h);

// functions for event handling
void do_event_loop();

// functions for hiding and showing the mouse
void hide_cursor();
void show_cursor();
// functions for hiding and showing the menubar
#define hide_menuber() ((void)0)
#define show_menubar() ((void)0)

// functions for blitting to the window
void bitblt_to_screen();
//extern void direct_to_screen();
//extern void copybits_to_screen();

// functions for hiding and showing the background
extern void black_window();
extern void hide_background();
extern void hide_foreground();

// screen shot saver
extern void SavePictScreen(int multiplayer);

// functions for offscreen compatibility stuff
extern grs_canvas *mac_get_gworld_data( int w, int h );

// global function that will quit application
extern void do_appl_quit( void );

extern int GetDiskInserted( void );

void SetEntries(int first, int last, color_record* pal);
void Index2Color(int index, color_record* color);

#endif
