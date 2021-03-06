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

#ifndef _AUTOMAP_H
#define _AUTOMAP_H

typedef struct Edge_info {
	short verts[2];		// 4 bytes
	ubyte sides[4];			// 4 bytes
	short segnum[4];			// 8 bytes	// This might not need to be stored... If you can access the normals of a side.
	ubyte flags;				// 1 bytes 	// See the EF_??? defines above.
	ubyte color;				// 1 bytes
	ubyte num_faces;			// 1 bytes	// 19 bytes...
} Edge_info;

extern Edge_info *Edges;

#define MAX_EDGES 6000		// Determined by loading all the levels by John & Mike, Feb 9, 1995

extern void do_automap(int key_code);
extern void automap_clear_visited();
extern ubyte Automap_visited[MAX_SEGMENTS];
//extern void modex_print_message(int x, int y, char *str);

#endif
