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
#include <ctype.h>

#include "dtypes.h"
#include "game.h"
#include "digi.h"
#include "kconfig.h"
#include "palette.h"
#include "input.h"
#include "args.h"
#include "player.h"
#include "mission.h"

#define MAX_CTB_LEN	512

typedef struct preferences {
	ubyte	digi_volume;
	ubyte	midi_volume;
	ubyte	stereo_reverse;
	ubyte	detail_level;
	ubyte	oc;					// object complexity
	ubyte	od;					// object detail
	ubyte	wd;					// wall detail
	ubyte	wrd;				// wall render depth
	ubyte	da;					// debris amount
	ubyte	sc;					// sound channels
	ubyte	gamma_level;
	ubyte	pixel_double;
	int		joy_axis_min[JOY_AXIS_COUNT];
	int		joy_axis_max[JOY_AXIS_COUNT];
	int		joy_axis_center[JOY_AXIS_COUNT];
	char	lastplayer[CALLSIGN_LEN+1];
	char	lastmission[MISSION_NAME_LEN+1];
	char	ctb_config[MAX_CTB_LEN];
	int		ctb_tool;
	ubyte	master_volume;
} Preferences;

char config_last_player[CALLSIGN_LEN+1] = "";
char config_last_mission[MISSION_NAME_LEN+1] = "";
char config_last_ctb_cfg[MAX_CTB_LEN] = "";
int config_last_ctb_tool;
ubyte Config_master_volume = 4;

int Config_vr_type = 0;
int Config_vr_tracking = 0;

extern byte	Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels;
extern void digi_set_master_volume( int volume );

void set_custom_detail_vars(void);

int ReadConfigFile()
{
	int i;
	FILE *prefs_handle;
	Preferences *prefs;
	char *p;

	prefs_handle = fopen("descent.cfg", "rb");
	if (prefs_handle == NULL)
		return -1;

	prefs = (Preferences *)malloc(sizeof(Preferences));
	if (prefs)
		i = fread(prefs, 1, sizeof(Preferences), prefs_handle);

	if (prefs==NULL || i < sizeof(Preferences)) {
		free(prefs);
		fclose(prefs_handle);
		return -1;
	}

	p = (char *)prefs;
	for (i = 0; i < sizeof(Preferences); i++) {
		if (*p != 0)
			break;
		p++;
	}
	if ( i == sizeof(Preferences) ) {
		free(prefs);
		fclose(prefs_handle);
		return -1;
	}

	Config_digi_volume = prefs->digi_volume;
	Config_midi_volume = prefs->midi_volume;
	Config_master_volume = prefs->master_volume;
	Config_channels_reversed = prefs->stereo_reverse;
	gr_palette_set_gamma( (int)(prefs->gamma_level) );
	Scanline_double = (int)prefs->pixel_double;
	Detail_level = prefs->detail_level;
	if (Detail_level == NUM_DETAIL_LEVELS-1) {
		Object_complexity = prefs->oc;
		Object_detail = prefs->od;
		Wall_detail = prefs->wd;
		Wall_render_depth = prefs->wrd;
		Debris_amount = prefs->da;
		SoundChannels = prefs->sc;
		set_custom_detail_vars();
	}
	strncpy( config_last_player, prefs->lastplayer, CALLSIGN_LEN );
	p = strchr(config_last_player, '\n' );
	if (p) *p = 0;

	strncpy(config_last_mission, prefs->lastmission, MISSION_NAME_LEN);
	p = strchr(config_last_mission, '\n' );
	if (p) *p = 0;

	strcpy(config_last_ctb_cfg, prefs->ctb_config);

	if ( Config_digi_volume > 8 ) Config_digi_volume = 8;

	if ( Config_midi_volume > 8 ) Config_midi_volume = 8;

	joy_set_cal_vals( prefs->joy_axis_min, prefs->joy_axis_center, prefs->joy_axis_max);
	digi_set_volume( (Config_digi_volume*256)/8, (Config_midi_volume*256)/8 );
	digi_set_master_volume(Config_master_volume);

	free(prefs);
	fclose(prefs_handle);
	return 0;
}

int WriteConfigFile()
{
	int i;
	FILE* prefs_handle;
	Preferences *prefs;

	prefs_handle = fopen("descent.cfg", "wb");
	if (prefs_handle == NULL) {
		return -1;
	}

	prefs = (Preferences*)malloc(sizeof(Preferences));
	if (prefs==NULL) {
		fclose(prefs_handle);
		return -1;
	}

	joy_get_cal_vals(prefs->joy_axis_min, prefs->joy_axis_center, prefs->joy_axis_max);
	prefs->digi_volume = Config_digi_volume;
	prefs->midi_volume = Config_midi_volume;
	prefs->stereo_reverse = Config_channels_reversed;
	prefs->detail_level = Detail_level;
	if (Detail_level == NUM_DETAIL_LEVELS-1) {
		prefs->oc = Object_complexity;
		prefs->od = Object_detail;
		prefs->wd = Wall_detail;
		prefs->wrd = Wall_render_depth;
		prefs->da = Debris_amount;
		prefs->sc = SoundChannels;
	}
	prefs->gamma_level = (ubyte)gr_palette_get_gamma();
	prefs->pixel_double = (ubyte)Scanline_double;
	strncpy( prefs->lastplayer, Players[Player_num].callsign, CALLSIGN_LEN );
	strncpy( prefs->lastmission, config_last_mission, MISSION_NAME_LEN );
	strcpy( prefs->ctb_config, config_last_ctb_cfg);
	prefs->ctb_tool = config_last_ctb_tool;
	prefs->master_volume = Config_master_volume;

	i = fwrite(prefs, sizeof(Preferences), 1, prefs_handle);
	fclose(prefs_handle);
	free(prefs);
	return i;
}

