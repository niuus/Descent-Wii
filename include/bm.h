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

#ifndef _BM_H
#define _BM_H

#include "gr.h"
#include "piggy.h"

#define MAX_TEXTURES		800
#define BM_MAX_ARGS		10

//tmapinfo flags
#define TMI_VOLATILE		1		//this material blows up when hit

typedef struct {
	char			filename[13];
	ubyte			flags;
	fix			lighting __attribute__((packed));		// 0 to 1
	fix			damage __attribute__((packed));			//how much damage being against this does
	int			eclip_num __attribute__((packed));		//if not -1, the eclip that changes this
} tmap_info;

extern int Num_object_types;

#define N_COCKPIT_BITMAPS 4
extern int Num_cockpits;
extern bitmap_index cockpit_bitmap[N_COCKPIT_BITMAPS];

extern int Num_tmaps;
#ifdef EDITOR
extern int TmapList[MAX_TEXTURES];
#endif

extern tmap_info TmapInfo[MAX_TEXTURES];

//for each model, a model number for dying & dead variants, or -1 if none
extern int Dying_modelnums[];
extern int Dead_modelnums[];

// Initializes the palette, bitmap system...
int bm_init();
void bm_close();

// Initializes the Texture[] array of bmd_bitmap structures.
void init_textures();

#define OL_ROBOT 				1
#define OL_HOSTAGE 			2
#define OL_POWERUP 			3
#define OL_CONTROL_CENTER	4
#define OL_PLAYER				5
#define OL_CLUTTER			6		//some sort of misc object
#define OL_EXIT				7		//the exit model for external scenes

#define	MAX_OBJTYPE			100

extern int Num_total_object_types;		//	Total number of object types, including robots, hostages, powerups, control centers, faces
extern byte	ObjType[MAX_OBJTYPE];		// Type of an object, such as Robot, eg if ObjType[11] == OL_ROBOT, then object #11 is a robot
extern byte	ObjId[MAX_OBJTYPE];			// ID of a robot, within its class, eg if ObjType[11] == 3, then object #11 is the third robot
extern fix	ObjStrength[MAX_OBJTYPE];	// initial strength of each object

#define MAX_OBJ_BITMAPS				210
extern bitmap_index ObjBitmaps[MAX_OBJ_BITMAPS];
extern ushort ObjBitmapPtrs[MAX_OBJ_BITMAPS];
extern int First_multi_bitmap_num;


#endif
