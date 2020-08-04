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

#ifndef _KCONFIG_H
#define _KCONFIG_H

typedef struct control_info {
	fix	pitch_time;
	fix	vertical_thrust_time;
	fix	heading_time;
	fix	sideways_thrust_time;
	fix	bank_time;
	fix	forward_thrust_time;

	ubyte	rear_view_down_count;
	ubyte	rear_view_down_state;

	ubyte	fire_primary_down_count;
	ubyte	fire_primary_state;
	ubyte	fire_secondary_state;
	ubyte	fire_secondary_down_count;
	ubyte	fire_flare_down_count;

	ubyte	drop_bomb_down_count;

	ubyte	automap_down_count;
	ubyte	automap_state;

	ubyte   cycle_primary;
	ubyte   cycle_secondary;

} control_info;

extern control_info Controls;
extern void controls_read_all();
extern void kconfig(int n, char * title );

extern ubyte Config_digi_volume;
extern ubyte Config_midi_volume;
extern ubyte Config_control_type;
extern ubyte Config_channels_reversed;
extern ubyte Config_joystick_sensitivity;

#define CONTROL_WIIMOTE 0
#define CONTROL_NUNCHUK 1
#define CONTROL_CLASSIC 2
#define CONTROL_MAX_TYPES 3

#define NUM_KEY_CONTROLS 46
#define NUM_OTHER_CONTROLS 30
#define MAX_CONTROLS 50

extern ubyte kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];
extern ubyte default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];

extern char *control_text[CONTROL_MAX_TYPES];

extern void kc_set_controls();

//set the cruise speed to zero
extern void reset_cruise(void);

extern int kconfig_is_axes_used(int axis);
extern int is_control_mapped(int index);

#endif
