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

#ifndef _DIGI_H
#define _DIGI_H

#include "dtypes.h"
#include "vecmat.h"

typedef struct digi_sound 	{
	int length;
	ubyte * data;
} digi_sound;

extern int digi_driver_board;
extern int digi_driver_port;
extern int digi_driver_irq;
extern int digi_driver_dma;
extern int digi_midi_type;
extern int digi_midi_port;
extern int digi_master_volume;		// master volume as defined by SM....we won't hold this value over between games

extern void digi_set_master_volume( int volume );

extern void digi_load_sounds();
extern int digi_get_settings();
extern int digi_init();
extern void digi_reset();
extern void digi_close();

// Volume is max at F1_0.
extern void digi_play_sample( int sndnum, fix max_volume );
extern void digi_play_sample_once( int sndnum, fix max_volume );
extern int digi_link_sound_to_object( int soundnum, short objnum, int forever, fix max_volume );
extern int digi_link_sound_to_pos( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume );
// Same as above, but you pass the max distance sound can be heard.  The old way uses f1_0*256 for max_distance.
extern int digi_link_sound_to_object2( int soundnum, short objnum, int forever, fix max_volume, fix  max_distance );
extern int digi_link_sound_to_pos2( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume, fix max_distance );
extern void digi_play_midi_song( const char * filename, const char * melodic_bank, const char * drum_bank, int loop );
extern void digi_play_sample_3d( int soundno, int angle, int volume, int no_dups ); // Volume from 0-0x7fff

extern void digi_init_sounds();
extern void digi_sync_sounds();

extern void digi_kill_sound_linked_to_segment( int segnum, int sidenum, int soundnum );
extern void digi_kill_sound_linked_to_object( int objnum );


extern void digi_set_midi_volume( int mvolume );
extern void digi_set_digi_volume( int dvolume );
extern void digi_set_volume( int dvolume, int mvolume );

extern int digi_is_sound_playing(int soundno);

extern void digi_pause_all();
extern void digi_resume_all();
extern void digi_stop_all();

extern void digi_set_max_channels(int n);
extern int digi_get_max_channels();

extern int digi_lomem;

#endif
