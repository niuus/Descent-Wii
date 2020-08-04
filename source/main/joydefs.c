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
#include <stdio.h>
#include <string.h>

#include "mono.h"
#include "input.h"
#include "error.h"
#include "gtimer.h"

#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"

#include "controls.h"
#include "joydefs.h"
#include "render.h"
#include "palette.h"
#include "newmenu.h"
#include "args.h"
#include "text.h"
#include "kconfig.h"
#include "digi.h"
#include "playsave.h"

int joydefs_calibrate_flag = 0;

void joy_delay()
{
	fix t1 = timer_get_fixed_seconds() + (F1_0/4);
	stop_time();
	while (t1 < timer_get_fixed_seconds());
	joy_flush();
	key_flush();
	start_time();

}


int joycal_message( char * title, char * text )
{
	int i;
	newmenu_item	m[2];
	m[0].type = NM_TYPE_TEXT; m[0].text = text;
//	m[1].type = NM_TYPE_MENU; m[1].text = TXT_OK;
	i = newmenu_do( title, NULL, 1, m, NULL );
	if ( i < 0 )
		return 1;
	return 0;
}

extern int WriteConfigFile();

void joydefs_calibrate()
{
	int org_axis_min[JOY_AXIS_COUNT];
	int org_axis_center[JOY_AXIS_COUNT];
	int org_axis_max[JOY_AXIS_COUNT];

	int axis_min[JOY_AXIS_COUNT] = { 0, 0, 0, 0};
	int axis_cen[JOY_AXIS_COUNT] = { 0, 0, 0, 0};
	int axis_max[JOY_AXIS_COUNT] = { 0, 0, 0, 0};

	int temp_values[JOY_AXIS_COUNT];
	char title[50];
	char text[50];
	int nsticks = 0;

	joydefs_calibrate_flag = 0;

	// wiimote config has nothing to calibrate
	if (   (Config_control_type!=CONTROL_NUNCHUK)
		&& (Config_control_type!=CONTROL_CLASSIC))
		return;

	joy_get_cal_vals(org_axis_min, org_axis_center, org_axis_max);

	joy_set_cen();
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );

//MWA	if (!joy_present)	{
//MWA		nm_messagebox( NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
//MWA		return;
//MWA	}

	// don't calibrate wiimote axes
	if (Config_control_type==CONTROL_NUNCHUK)
		nsticks = 1;
	else
		nsticks = 2;

	if ( nsticks == 2 )	{
		sprintf( title, "LEFT JOYSTICK\n%s", TXT_UPPER_LEFT);
		sprintf( text, "Move Left Joystick %s", TXT_TO_UL);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_UPPER_LEFT);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_UL);
	}
	if (joycal_message( title, text )) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_min[0] = temp_values[0];
	axis_max[1] = temp_values[1];
	joy_delay();

	if ( nsticks == 2 )	{
		sprintf( title, "LEFT JOYSTICK\n%s", TXT_LOWER_RIGHT);
		sprintf( text, "Move Left Joystick %s", TXT_TO_LR);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_LOWER_RIGHT);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_LR);
	}
	if (joycal_message( title, text)) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_max[0] = temp_values[0];
	axis_min[1] = temp_values[1];
	joy_delay();

	if ( nsticks == 2 )	{
		sprintf( title, "LEFT_JOYSTICK\n%s", TXT_CENTER);
		sprintf( text, "Move Left Joystick %s", TXT_TO_C);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_CENTER);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_C);
	}
	if (joycal_message( title, text)) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_cen[0] = temp_values[0];
	axis_cen[1] = temp_values[1];
	joy_delay();

	if (nsticks==2 && (kconfig_is_axes_used(2) || kconfig_is_axes_used(3))) {
		sprintf( title, "RIGHT JOYSTICK\n%s", TXT_UPPER_LEFT);
		sprintf( text, "Move Right Joystick %s", TXT_TO_UL);
		if (joycal_message( title, text )) {
			joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
			return;
		}
		joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
		axis_min[2] = temp_values[2];
		axis_max[3] = temp_values[3];
		joy_delay();

		sprintf( title, "RIGHT JOYSTICK\n%s", TXT_LOWER_RIGHT);
		sprintf( text, "Move Right Joystick %s", TXT_TO_LR);
		if (joycal_message( title, text ))	{
			joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
			return;
		}
		joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
		axis_max[2] = temp_values[2];
		axis_min[3] = temp_values[3];
		joy_delay();

		sprintf( title, "RIGHT JOYSTICK\n%s", TXT_CENTER);
		sprintf( text, "Move Right Joystick %s", TXT_TO_C);
		if (joycal_message( title, text ))	{
			joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
			return;
		}
		joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
		axis_cen[2] = temp_values[2];
		axis_cen[3] = temp_values[3];
		joy_delay();
	}

	joy_set_cal_vals(axis_min, axis_cen, axis_max);

	WriteConfigFile();
}

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int oc_type = Config_control_type;

	if (items[0].value)
		Config_control_type = CONTROL_WIIMOTE;
	else if (items[1].value) {
		if (joy_init() != JOY_AS_NUNCHUK) { // not connected
			items[1].value = 0;
			items[1].redraw = 1;
			items[oc_type].value = 1;
			items[oc_type].redraw = 1;
		}
		else
			Config_control_type = CONTROL_NUNCHUK;
	}
	else if (items[2].value) {
		if (joy_init() != JOY_AS_CLASSIC) { // not connected
			items[2].value = 0;
			items[2].redraw = 1;
			items[oc_type].value = 1;
			items[oc_type].redraw = 1;
		}
		else
			Config_control_type = CONTROL_CLASSIC;
	}
	else {
		Int3();
		Config_control_type = CONTROL_WIIMOTE;
	}

	if (oc_type != Config_control_type) {
		switch (Config_control_type) {
			case	CONTROL_NUNCHUK:
			case	CONTROL_CLASSIC:
				joydefs_calibrate_flag = 1;
		}
		kc_set_controls();
		joydefs_set_type(Config_control_type);
	}
}

void joydefs_config()
{
	int i;
	newmenu_item m[CONTROL_MAX_TYPES+2];
	int i1 = CONTROL_MAX_TYPES+1; // default selection is customize

	for (i=0; i < CONTROL_MAX_TYPES; i++) {
		m[i].type = NM_TYPE_RADIO; m[i].text = CONTROL_TEXT(i); m[i].value = 0; m[i].group = 0;
	}

	m[i].type = NM_TYPE_TEXT; m[i++].text="";
	m[i].type = NM_TYPE_MENU; m[i++].text=TXT_CUST_ABOVE;

	if (Config_control_type >= 0 && Config_control_type < CONTROL_MAX_TYPES)
		m[Config_control_type].value = 1;
	else
		m[0].value = 1;

	Config_control_type = CONTROL_WIIMOTE;
	joydef_menuset_1(i, m, NULL, 0);

	do {
		i1 = newmenu_do4( NULL, TXT_CONTROLS, i, m, joydef_menuset_1, i1, NULL, -1, -1, 1 );

		if (i1 == CONTROL_MAX_TYPES+1) // customize
			kconfig(Config_control_type, CONTROL_TEXT(Config_control_type));

	} while(i1>-1);

}

void joydefs_set_type(ubyte type)
{
	ubyte joy_type;

	switch (type)
	{
		default:
		case	CONTROL_WIIMOTE:			joy_type = JOY_AS_NONE;					break;
		case	CONTROL_NUNCHUK:			joy_type = JOY_AS_NUNCHUK;				break;
		case	CONTROL_CLASSIC:			joy_type = JOY_AS_CLASSIC;				break;
	}
	joy_set_type(joy_type);
}
