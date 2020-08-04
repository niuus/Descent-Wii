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

#ifndef _INPUT_H
#define _INPUT_H

#include "dtypes.h"
#include "fix.h"

// might use these one day
#define MB_LEFT				0
#define MB_RIGHT			1
#define MB_MIDDLE			2
#define MB_Z_UP				3
#define MB_Z_DOWN			4
#define MB_PITCH_BACKWARD	5
#define MB_PITCH_FORWARD 	6
#define MB_BANK_LEFT		7
#define MB_BANK_RIGHT		8
#define MB_HEAD_LEFT		9
#define MB_HEAD_RIGHT		10
#define MOUSE_MAX_BUTTONS   11

// wiimote A/classic A
#define JOY_BUTTON_A        (1 << 0)
// wiimote B/classic B
#define JOY_BUTTON_B        (1 << 1)
// wiimote -/classic -
#define JOY_BUTTON_MINUS    (1 << 2)
// wiimote +/classic +
#define JOY_BUTTON_PLUS     (1 << 3)
// classic L
#define JOY_BUTTON_L        (1 << 4)
// classic R
#define JOY_BUTTON_R        (1 << 5)
// classic Zl
#define JOY_BUTTON_ZL       (1 << 6)
// classic Zr
#define JOY_BUTTON_ZR       (1 << 7)
// classic X
#define JOY_BUTTON_X        (1 << 8)
// classic Y
#define JOY_BUTTON_Y        (1 << 9)
// wiimote 1
#define JOY_BUTTON_1        (1 << 10)
// wiimote 2
#define JOY_BUTTON_2        (1 << 11)
// nunchuk C
#define JOY_BUTTON_C        (1 << 12)
// nunchuk Z
#define JOY_BUTTON_Z        (1 << 13)
// DPAD up
#define JOY_BUTTON_UP       (1 << 14)
// DPAD down
#define JOY_BUTTON_DOWN     (1 << 15)
// DPAD left
#define JOY_BUTTON_LEFT     (1 << 16)
// DPAD right
#define JOY_BUTTON_RIGHT    (1 << 17)

#define JOY_BUTTON_COUNT    18
#define JOY_ALL_BUTTONS		((1<<JOY_BUTTON_COUNT)-1)

// nunchuk stick/classic L joy
#define JOY_1_X_AXIS		(1 << 0)
#define JOY_1_Y_AXIS		(1 << 1)
// classic R joy
#define JOY_2_X_AXIS		(1 << 2)
#define JOY_2_Y_AXIS		(1 << 3)
#define JOY_AXIS_COUNT      4
#define JOY_ALL_AXIS		((1<<JOY_AXIS_COUNT)-1)

// use no joystick/extension
#define JOY_AS_NONE			0
// use wiimote+nunchuk
#define JOY_AS_NUNCHUK      1
// use classic
#define JOY_AS_CLASSIC      2

#define KEY_SHIFTED			(1 << 8)
#define KEY_ALTED			(1 << 9)
#define KEY_CTRLED			(1 << 10)
#define KEY_DEBUGGED		(1 << 11)
#define KEY_COMMAND			(1 << 12)

#define KEY_0           	0x1D
#define KEY_1           	0x12
#define KEY_2           	0x13
#define KEY_3           	0x14
#define KEY_4           	0x15
#define KEY_5           	0x17
#define KEY_6           	0x16
#define KEY_7           	0x1A
#define KEY_8           	0x1C
#define KEY_9           	0x19

#define KEY_A           	0x3F
#define KEY_B           	0x0B
#define KEY_C           	0x08
#define KEY_D           	0x02
#define KEY_E           	0x0E
#define KEY_F           	0x03
#define KEY_G           	0x05
#define KEY_H           	0x04
#define KEY_I           	0x22
#define KEY_J           	0x26
#define KEY_K           	0x28
#define KEY_L           	0x25
#define KEY_M           	0x2E
#define KEY_N           	0x2D
#define KEY_O           	0x1F
#define KEY_P           	0x23
#define KEY_Q           	0x0C
#define KEY_R           	0x0F
#define KEY_S           	0x01
#define KEY_T           	0x11
#define KEY_U           	0x20
#define KEY_V           	0x09
#define KEY_W           	0x0D
#define KEY_X           	0x07
#define KEY_Y           	0x10
#define KEY_Z           	0x06

#define KEY_MINUS       	0x1B
#define KEY_EQUAL       	0x18
#define KEY_DIVIDE      	0x2C
#define KEY_SLASH       	0x2A
#define KEY_COMMA       	0x2B
#define KEY_PERIOD      	0x2F
#define KEY_SEMICOL     	0x29

#define KEY_LBRACKET    	0x21
#define KEY_RBRACKET    	0x1E

#define KEY_RAPOSTRO    	0x27
#define KEY_LAPOSTRO    	0x32

// wiimote/classic HOME
#define KEY_ESC         	0x35
#define KEY_ENTER       	0x24
#define KEY_BACKSP      	0x33
#define KEY_TAB         	0x30
#define KEY_SPACEBAR    	0x31

#define KEY_NUMLOCK     	0x47
#define KEY_SCROLLOCK   	0x6B
#define KEY_CAPSLOCK    	0x39

#define KEY_LSHIFT      	0x38
#define KEY_RSHIFT      	0x38

#define KEY_LALT        	0x3A
#define KEY_RALT        	0x3A

#define KEY_LCTRL       	0x3B
#define KEY_RCTRL       	0x3B

#define KEY_CMD				0x37

#define KEY_F1          	0x7A
#define KEY_F2          	0x78
#define KEY_F3          	0x63
#define KEY_F4          	0x76
#define KEY_F5          	0x60
#define KEY_F6          	0x61
#define KEY_F7          	0x62
#define KEY_F8          	0x64
#define KEY_F9          	0x65
#define KEY_F10         	0x6D
#define KEY_F11         	0x67
#define KEY_F12         	0x6F

#define KEY_PAD0        	0x52
#define KEY_PAD1        	0x53
#define KEY_PAD2        	0x54
#define KEY_PAD3        	0x55
#define KEY_PAD4        	0x56
#define KEY_PAD5        	0x57
#define KEY_PAD6        	0x58
#define KEY_PAD7        	0x59
#define KEY_PAD8        	0x5B
#define KEY_PAD9        	0x5C
#define KEY_PADMINUS    	0x4E
#define KEY_PADPLUS     	0x45
#define KEY_PADPERIOD   	0x41
#define KEY_PADDIVIDE   	0x4B
#define KEY_PADMULTIPLY 	0x43
#define KEY_PADENTER    	0x4C

#define KEY_INSERT      	0x72
#define KEY_HOME        	0x73
#define KEY_PAGEUP      	0x74
#define KEY_DELETE      	0x75
#define KEY_END         	0x77
#define KEY_PAGEDOWN    	0x79
#define KEY_UP          	0x7E
#define KEY_DOWN        	0x7D
#define KEY_LEFT        	0x7B
#define KEY_RIGHT       	0x7C

#define KEY_PRINT_SCREEN	0x69
#define KEY_PAUSE			0x71

//==========================================================================
// This initializes the joy and does a "quick" calibration which
// assumes the stick is centered and sets the minimum value to 0 and
// the maximum value to 2 times the centered reading. Returns 0 if no
// joystick was detected, 1 if everything is ok.
extern int joy_init();
extern void joy_set_cen();

//==========================================================================
// This just reads the buttons and returns their status.  When bit 0
// is 1, button 1 is pressed, when bit 1 is 1, button 2 is pressed.
extern int joy_get_btns();

//==========================================================================
// This returns the number of times a button went either down or up since
// the last call to this function.
extern int joy_get_button_down_cnt( int btn );

//==========================================================================
// This returns how long (in approximate milliseconds) that each of the
// buttons has been held down since the last call to this function.
// It is the total time... say you pressed it down for 3 ticks, released
// it, and held it down for 6 more ticks. The time returned would be 9.
extern fix joy_get_button_down_time( int btn );

extern ubyte joystick_read_raw_axis( ubyte mask, int * axis );
extern void joy_flush();
extern ubyte joy_get_present_mask();

extern int joy_get_button_state( int btn );
extern void joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max);
extern void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max);
extern int joy_get_scaled_reading( int raw, int axn );

extern void joy_set_type(ubyte type);

//==========================================================================
// This installs the int9 vector and initializes the keyboard in buffered
// ASCII mode. key_close simply undoes that.
extern void key_init();
extern void key_close();

extern unsigned char keyd_repeat;        // 1=allow repeating, 0=dont allow repeat

// Time in seconds when last key was pressed...
extern volatile int keyd_time_when_last_pressed;

//==========================================================================
// These are the "buffered" keypress routines.  Use them by setting the
// "keyd_buffer_type" variable.

// clear down_time/down_count values in flush
extern void key_flush();    // Clears the 256 char buffer
extern int key_inkey();     // Gets key if one, other returns 0.
extern int key_inkey_time(fix *time);     // Same as inkey, but returns the time the key was pressed down.
extern int key_peekkey();   // Same as inkey, but doesn't remove key from buffer.
extern unsigned int key_get_shift_status();

extern unsigned char key_to_ascii(int keycode );

//==========================================================================
// These are the unbuffered routines. Index by the keyboard scancode.

// Set to 1 if the key is currently down, else 0
extern volatile unsigned char keyd_pressed[];

// Returns the seconds this key has been down since last call.
extern fix key_down_time(int scancode);

// Returns number of times key has went from up to down since last call.
extern unsigned int key_down_count(int scancode);

//========================================================================
// Check for mouse driver, reset driver if installed.

extern int mouse_init();
extern void mouse_flush();	// clears all mice events...

//========================================================================
extern void mouse_get_pos( int *x, int *y);
extern void mouse_get_delta( int *dx, int *dy );
extern int mouse_get_btns();

// Returns how long this button has been down since last call.
extern fix mouse_button_down_time(int button);

// Returns how many times this button has went down since last call.
extern int mouse_button_down_count(int button);

// Returns 1 if this button is currently down
extern int mouse_button_state(int button);

// return 1 if the button went down since last call
extern int mouse_went_down(int button);


#endif
