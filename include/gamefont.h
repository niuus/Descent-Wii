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

#ifndef _GAMEFONT_H
#define _GAMEFONT_H

// When adding a new font, don't forget to change the
// filename in gamefont.c!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define GFONT_BIG_1		0		// because we don't have the Big font on the mac -- a waste
								// use the first medium font.  Probably won't make a big difference
#define GFONT_MEDIUM_1	1
#define GFONT_MEDIUM_2	2
#define GFONT_MEDIUM_3	3
#define GFONT_SMALL		4

#define GAME_FONT		(Gamefonts[GFONT_SMALL])
#define HELP_FONT		(Gamefonts[GFONT_MEDIUM_1])
#define MENU_FONT		(Gamefonts[GFONT_MEDIUM_1])
#define SCORES_FONT		(Gamefonts[GFONT_MEDIUM_1])

#define MAX_FONTS 5

extern grs_font *Gamefonts[MAX_FONTS];

void gamefont_init();
void gamefont_close();


#endif