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
#include <unistd.h>
#include <gccore.h>
#include <ogcsys.h>
#include <wiiuse/wpad.h>

// dtypes.h conflicts with wiiuse.h
#define _TYPES_H
typedef unsigned long ulong;

#include "input.h"
#include "gtimer.h"
#include "error.h"

#define KEY_BUFFER_SIZE 16
#define KEYBOARD_TICKS  40          // 40 times a second
#define KEYBOARD_TIME_NS	(1e9/KEYBOARD_TICKS)
#define KEY_THREAD_PRIO			65

extern ubyte cursor_hidden;

typedef struct Key_info {
	ubyte		state;			// state of key 1 == down, 0 == up
	ubyte		last_state;		// previous state of key
	int			counter;		// incremented each time key is down in handler
	fix			timewentdown;	// simple counter incremented each time in interrupt and key is down
	fix			timehelddown;	// counter to tell how long key is down -- gets reset to 0 by key routines
	ubyte		downcount;		// number of key counts key was down
	ubyte		upcount;		// number of times key was released
} Key_info;

static struct keyboard_info	{
	unsigned short		keybuffer[KEY_BUFFER_SIZE];
	Key_info			keys[128];
	fix					time_pressed[KEY_BUFFER_SIZE];
	unsigned int 		keyhead, keytail;
	u32					exp_type;
} Keyboard;

static struct mouse_info {
	ubyte	pressed[MOUSE_MAX_BUTTONS];
	fix		time_held_down[MOUSE_MAX_BUTTONS];
	uint	num_downs[MOUSE_MAX_BUTTONS];
	uint	num_ups[MOUSE_MAX_BUTTONS];
	ubyte	went_down[MOUSE_MAX_BUTTONS];
	int     mouse_x, mouse_y;
} Mouse;

static struct joy_info {
	int			axis_min[JOY_AXIS_COUNT];
	int			axis_center[JOY_AXIS_COUNT];
	int			axis_max[JOY_AXIS_COUNT];
	int			wrote_cal;
	int			co_ords[JOY_AXIS_COUNT];
	int			buttons;
	int			old_buttons;
	int			button_down_count[JOY_BUTTON_COUNT];
	fix			button_down_tick[JOY_BUTTON_COUNT];
} Joystick;

//-------- Variables accessed by local functions ----------
static mutex_t kb_mutex = LWP_MUTEX_NULL;
static lwp_t kb_thread = LWP_THREAD_NULL;
static unsigned char Installed=0;
#ifdef __wii__
static char joy_type = JOY_AS_NONE;
#else
static char joy_type = JOY_AS_CLASSIC;
#endif

//-------- Variables accessed by outside functions ---------
unsigned char           keyd_repeat;
volatile unsigned char	keyd_pressed[128];
volatile int			keyd_time_when_last_pressed;


static const unsigned char ascii_table[128] =
{ 255, 's', 'd', 'f', 'h', 'g', 'z', 'x', 'c', 'v', 255, 'b', 'q', 'w', 'e', 'r',
  'y', 't', '1', '2', '3', '4', '6', '5', '=', '9', '7', '-', '8', '0', ']', 'o',
  'u', '[', 'i', 'p', 255, 'l', 'j', 39, 'k', ';', '\\', ',', '/', 'n', 'm', '.',
  255, ' ', '`', 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'a',
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

static const unsigned char shifted_ascii_table[128] =
{ 255, 'S', 'D', 'F', 'H', 'G', 'Z', 'X', 'C', 'V', 255, 'B', 'Q', 'W', 'E', 'R',
  'Y', 'T', '!', '@', '#', '$', '^', '%', '+', '(', '&', '_', '*', ')', '}', 'O',
  'U', '{', 'I', 'P', 255, 'L', 'J', '"', 'K', ':', '|', '<', '?', 'N', 'M', '>',
  255, ' ', '~', 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 'A',
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

// mouse buttons are currently unused
static void mouse_button_handle(int button, const fix tick, int down) {
	if (down) {
		Mouse.went_down[button] = 1;
		if (!Mouse.pressed[button]) {
			Mouse.num_downs[button]++;
			Mouse.pressed[button] = 1;
			if (Mouse.time_held_down[button]==0)
				Mouse.time_held_down[button] = tick;
			else // account for time already held
				Mouse.time_held_down[button] = tick - Mouse.time_held_down[button];
		}
	} else if (Mouse.pressed[button]) {
		Mouse.num_ups[button]++;
		Mouse.pressed[button] = 0;
		Mouse.time_held_down[button] = tick - Mouse.time_held_down[button];
	}
}

static void keyboard_button_handle(int button, const fix tick, int down) {
	unsigned char next_tail;
	Key_info *key = Keyboard.keys+button;
	int key_state = !!down;
	if (key_state == key->last_state) {
		if (key_state) {
			key->counter++;
			keyd_time_when_last_pressed = tick;
		} // else it wasn't pressed before, and not pressed now. Do nothing.
	} else if (key_state) { // went down
		keyd_pressed[button] = 1;
		key->downcount++;
		key->state = 1;
		key->timewentdown = keyd_time_when_last_pressed = tick;
		key->counter=1;
	} else { // released
		keyd_pressed[button] = 0;
		key->upcount++;
		key->state = 0;
		key->counter = 0;
		key->timehelddown += tick - key->timewentdown;
	}
	// check if this key just went down or if it's repeating (every odd tick after being held for 1s)
	if ((key_state && !key->last_state) || (keyd_repeat && key_state && key->last_state && (tick-key->timewentdown > F1_0) && (key->counter & 1))) {
		if ( keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT])
			button |= KEY_SHIFTED;
		if ( keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT])
			button |= KEY_ALTED;
		if ( keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL])
			button |= KEY_CTRLED;
#ifndef NDEBUG
		if ( keyd_pressed[KEY_DELETE] )
			button |= KEY_DEBUGGED;
#endif
		next_tail = (Keyboard.keytail+1) % KEY_BUFFER_SIZE;
		if (next_tail != Keyboard.keyhead) {
			Keyboard.keybuffer[Keyboard.keytail] = button;
			Keyboard.time_pressed[Keyboard.keytail] = keyd_time_when_last_pressed;
			Keyboard.keytail = next_tail;
		} else
			Warning("Keyboard buffer overflow\n");
	}
	key->last_state = key_state;
}

static void gc_joystick_button_handle(const PADStatus *pd, u32 pad_button, u32 code)
{
	if (!(pd->button & pad_button))
		return;

	Joystick.buttons |= (1<<code);
	//Joystick.button_held_count[code]++;
	if (!(Joystick.old_buttons&(1<<code)))
		Joystick.button_down_count[code]++;
}

static void handle_gcpad(const PADStatus *pd, const fix tick) {
	if (Joystick.wrote_cal<5 || !Joystick.axis_max[0]) {
		Joystick.wrote_cal++;

		Joystick.axis_max[0]    = 127;
		// assume the sticks are centered and use their current pos values
		// classic controllers don't hold any "real" calibration data
		Joystick.axis_center[0] = pd->stickX;
		Joystick.axis_min[0]    = -127;

		Joystick.axis_max[1]    = 127;
		Joystick.axis_center[1] = pd->stickY;
		Joystick.axis_min[1]    = -127;

		Joystick.axis_max[2]    = 127;
		Joystick.axis_center[2] = pd->substickX;
		Joystick.axis_min[2]    = -127;

		Joystick.axis_max[3]    = 127;
		Joystick.axis_center[3] = pd->substickY;
		Joystick.axis_min[3]    = -127;
	}

	Joystick.co_ords[0] = pd->stickX;
	Joystick.co_ords[1] = pd->stickY;
	Joystick.co_ords[2] = pd->substickX;
	Joystick.co_ords[3] = pd->substickY;

	// joystick button translations
	gc_joystick_button_handle(pd, PAD_BUTTON_A,         0);
	gc_joystick_button_handle(pd, PAD_BUTTON_B,         1);
	gc_joystick_button_handle(pd, PAD_BUTTON_MENU,      2);
	gc_joystick_button_handle(pd, PAD_BUTTON_START,     3);
	gc_joystick_button_handle(pd, PAD_TRIGGER_L,        4);
	gc_joystick_button_handle(pd, PAD_TRIGGER_R,        5);
	gc_joystick_button_handle(pd, PAD_BUTTON_X,         8);
	gc_joystick_button_handle(pd, PAD_BUTTON_Y,         9);
	gc_joystick_button_handle(pd, PAD_BUTTON_UP,       14);
	gc_joystick_button_handle(pd, PAD_BUTTON_DOWN,     15);
	gc_joystick_button_handle(pd, PAD_BUTTON_LEFT,     16);
	gc_joystick_button_handle(pd, PAD_BUTTON_RIGHT,    17);

	// keyboard button translations
	keyboard_button_handle(KEY_ESC, tick, pd->button & PAD_TRIGGER_Z);

	// mouse button translations (menu control only)
	mouse_button_handle(MB_LEFT, tick, (pd->button&PAD_BUTTON_A) && !(Joystick.old_buttons&1));
	Joystick.old_buttons = Joystick.buttons;
}

#ifdef __wii__
static void joystick_button_handle(const WPADData *wd, u32 wm_button, u32 code, const fix tick)
{
	if (wd->btns_u & wm_button)
		Joystick.button_down_tick[code] = tick - Joystick.button_down_tick[code];

	if (!((wd->btns_d|wd->btns_h)&wm_button))
		return;

	Joystick.buttons |= (1<<code);
	if (wd->btns_d&wm_button) {
		Joystick.button_down_count[code]++;
		Joystick.button_down_tick[code] = tick;
	}
}

static void handle_classic(const WPADData *wd, const fix tick) {
	if (Joystick.wrote_cal<5 || !Joystick.axis_max[0]) {
		Joystick.wrote_cal++;

		Joystick.axis_max[0]    = wd->exp.classic.ljs.max.x;
		// assume the sticks are centered and use their current pos values
		// classic controllers don't hold any "real" calibration data
		Joystick.axis_center[0] = wd->exp.classic.ljs.pos.x;
		Joystick.axis_min[0]    = wd->exp.classic.ljs.min.x;

		Joystick.axis_max[1]    = wd->exp.classic.ljs.max.y;
		Joystick.axis_center[1] = wd->exp.classic.ljs.pos.y;
		Joystick.axis_min[1]    = wd->exp.classic.ljs.min.y;

		Joystick.axis_max[2]    = wd->exp.classic.rjs.max.x;
		Joystick.axis_center[2] = wd->exp.classic.rjs.pos.x;
		Joystick.axis_min[2]    = wd->exp.classic.rjs.min.x;

		Joystick.axis_max[3]    = wd->exp.classic.rjs.max.y;
		Joystick.axis_center[3] = wd->exp.classic.rjs.pos.y;
		Joystick.axis_min[3]    = wd->exp.classic.rjs.min.y;
	}

	Joystick.co_ords[0] = wd->exp.classic.ljs.pos.x;
	Joystick.co_ords[1] = wd->exp.classic.ljs.pos.y;
	Joystick.co_ords[2] = wd->exp.classic.rjs.pos.x;
	Joystick.co_ords[3] = wd->exp.classic.rjs.pos.y;

	// joystick button translations
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_A,      0, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_B,      1, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_MINUS,  2, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_PLUS,   3, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_FULL_L, 4, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_FULL_R, 5, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_ZL,     6, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_ZR,     7, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_X,      8, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_Y,      9, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_UP,    14, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_DOWN,  15, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_LEFT,  16, tick);
	joystick_button_handle(wd, WPAD_CLASSIC_BUTTON_RIGHT, 17, tick);

	// keyboard button translations
	keyboard_button_handle(KEY_ESC, tick, (wd->btns_d|wd->btns_h) & (WPAD_CLASSIC_BUTTON_HOME|WPAD_BUTTON_HOME));

	// mouse button translations (menu control only)
	mouse_button_handle(MB_LEFT, tick, wd->btns_d & WPAD_CLASSIC_BUTTON_A);
}

static void handle_nunchuk(const WPADData *wd, const fix tick) {
	if (Joystick.wrote_cal<5 || !Joystick.axis_max[0]) {
		Joystick.wrote_cal++;

		Joystick.axis_max[0]    = wd->exp.nunchuk.js.max.x;
		Joystick.axis_center[0] = wd->exp.nunchuk.js.center.x;
		Joystick.axis_min[0]    = wd->exp.nunchuk.js.min.x;

		Joystick.axis_max[1]    = wd->exp.nunchuk.js.max.y;
		Joystick.axis_center[1] = wd->exp.nunchuk.js.center.y;
		Joystick.axis_min[1]    = wd->exp.nunchuk.js.min.y;
	}

	Joystick.co_ords[0] = wd->exp.nunchuk.js.pos.x;
	Joystick.co_ords[1] = wd->exp.nunchuk.js.pos.y;

	// joystick button translation
	joystick_button_handle(wd, WPAD_NUNCHUK_BUTTON_C, 12, tick);
	joystick_button_handle(wd, WPAD_NUNCHUK_BUTTON_Z, 13, tick);
}

static void handle_wiimote(const WPADData *wd, const fix tick) {
	u32 btns = wd->btns_d|wd->btns_h;

	// pointer
	if (wd->ir.valid) {
		Mouse.mouse_x = wd->ir.x;
		Mouse.mouse_y = wd->ir.y;
	} else { // offscreen
		Mouse.mouse_x = 320;
		Mouse.mouse_y = 200;
	}

	Joystick.co_ords[2] = wd->orient.roll;
	Joystick.co_ords[3] = wd->orient.pitch;

	// joystick button translation
	joystick_button_handle(wd, WPAD_BUTTON_A,      0, tick);
	joystick_button_handle(wd, WPAD_BUTTON_B,      1, tick);
	joystick_button_handle(wd, WPAD_BUTTON_MINUS,  2, tick);
	joystick_button_handle(wd, WPAD_BUTTON_PLUS,   3, tick);
	joystick_button_handle(wd, WPAD_BUTTON_1,     10, tick);
	joystick_button_handle(wd, WPAD_BUTTON_2,     11, tick);
	joystick_button_handle(wd, WPAD_BUTTON_UP,    14, tick);
	joystick_button_handle(wd, WPAD_BUTTON_DOWN,  15, tick);
	joystick_button_handle(wd, WPAD_BUTTON_LEFT,  16, tick);
	joystick_button_handle(wd, WPAD_BUTTON_RIGHT, 17, tick);

	// keyboard button translations
	keyboard_button_handle(KEY_ESC, tick, btns & WIIMOTE_BUTTON_HOME);
	// used for dumping key configuration
	//keyboard_button_handle(KEY_A, tick, btns & WIIMOTE_BUTTON_ONE);

	mouse_button_handle(MB_LEFT, tick, wd->btns_d & WIIMOTE_BUTTON_A);
}
#endif

// keyboard handler which runs KEY_TIMER times a second.
static u32 keyboard_handler()
{
	const fix tick_now = timer_get_fixed_seconds();
	PADStatus pad_status[PAD_CHANMAX];
	u8 use_joy = joy_type;

	PAD_Read(pad_status);

#ifdef __wii__
	const WPADData *wd;
	int old_co_ords[6];

	old_co_ords[0] = Joystick.co_ords[0];
	old_co_ords[1] = Joystick.co_ords[1];
	old_co_ords[2] = Joystick.co_ords[2];
	old_co_ords[3] = Joystick.co_ords[3];
	old_co_ords[4] = Mouse.mouse_x;
	old_co_ords[5] = Mouse.mouse_y;

	if (WPAD_ReadPending(WPAD_CHAN_0, NULL)==0)
		return 0;
	wd = WPAD_Data(WPAD_CHAN_0);

	// this happens when the wiimote disconnects
	if (wd->data_present==0)
		return 0;

	if (Keyboard.exp_type != wd->exp.type) {
		Joystick.wrote_cal = 0;
		Joystick.axis_max[0] = 0;
		Keyboard.exp_type = WPAD_EXP_NONE;
		switch (wd->exp.type) {
			case WPAD_EXP_CLASSIC:
				Keyboard.exp_type = WPAD_EXP_CLASSIC;
				break;
			case WPAD_EXP_NUNCHUK:
				Keyboard.exp_type = WPAD_EXP_NUNCHUK;
			default:
				Joystick.axis_max[2]    = Joystick.axis_max[3]    = 45;
				Joystick.axis_min[2]    = Joystick.axis_min[3]    = -45;
				Joystick.axis_center[2] = Joystick.axis_center[3] = 0;
		}
	}

	if (Keyboard.exp_type==WPAD_EXP_NONE ||
	   (joy_type==JOY_AS_NUNCHUK && Keyboard.exp_type!=WPAD_EXP_NUNCHUK) ||
	   (joy_type==JOY_AS_CLASSIC && Keyboard.exp_type!=WPAD_EXP_CLASSIC)) {
		   use_joy = JOY_AS_NONE;
	}
#endif

	Joystick.buttons = 0;
#ifdef __wii__
	switch (use_joy) {
		case JOY_AS_CLASSIC:
			//handle_gcpad(pad_status, tick_now);
			handle_classic(wd, tick_now);
			break;
		case JOY_AS_NUNCHUK:
			handle_nunchuk(wd, tick_now);
		case JOY_AS_NONE:
		default:
			handle_wiimote(wd, tick_now);
	}

	// check button states
	if (wd->btns_h|wd->btns_d)
	{
//		printf("Joystick is active; buttons changed\n");
		return 1;
	}
	// check for stick movement
	if (old_co_ords[0] != Joystick.co_ords[0] || old_co_ords[1] != Joystick.co_ords[1])
	{
//		printf("Joystick is active; stick movement\n");
		return 1;
	}
	// check for substick movement
	if (use_joy==JOY_AS_CLASSIC &&
	   (old_co_ords[2] != Joystick.co_ords[2] || old_co_ords[3] != Joystick.co_ords[3]))
	{
//		printf("Joystick is active; substick movement\n");
		return 1;
	}
	// check for accelerometer movement
	if (use_joy!=JOY_AS_CLASSIC &&
	   (abs(old_co_ords[2] - Joystick.co_ords[2]) >= 10 || abs(old_co_ords[3] - Joystick.co_ords[3]) >= 10))
	{
//		printf("Joystick is active: accelerometer\n");
	    return 1;
	}
	// check for IR movement
	if (old_co_ords[4] != Mouse.mouse_x || old_co_ords[5] != Mouse.mouse_y)
	{
//		printf("Joystick is active; IR changed\n");
		return 1;
	}
	// input is idle
	return 0;
#else
	handle_gcpad(pad_status, tick_now);
	// GC doesn't do dimming, return always active
	return 1;
#endif
}

static void keyboard_tick(syswd_t s, lwp_t thread)
{
	LWP_ResumeThread(thread);
}

static void VI_Dim(int enable)
{
#ifdef __wii__
	int fd;
	u32 immbufin[8] ATTRIBUTE_ALIGN(32);
	u32 immbufout[8] ATTRIBUTE_ALIGN(32);

	if (CONF_GetScreenSaverMode() != 1)
	{
//		printf("Screen Burn-In Reduction is disabled, skipping dimming\n");
		return;
	}

	fd = IOS_Open("/dev/stm/immediate", 0);
	if (fd<0)
		return;

	immbufin[0] = (2<<3)|2;
	immbufin[4] = 0;
	immbufin[5] = immbufin[6] = 0xFFFFFFFF;
	if (enable)
		immbufin[0] |= (1<<7);

	IOS_Ioctl(fd, 0x5001, immbufin, sizeof(immbufin), immbufout, sizeof(immbufout));
	IOS_Close(fd);
#endif
}

void* keyboard_thread(void *x)
{
	struct timespec tm = {0, KEYBOARD_TIME_NS};
	syswd_t alarm;
	SYS_CreateAlarm(&alarm);
	fix dim_timer = timer_get_fixed_seconds();
	int is_dim = 0;

	while (!LWP_MutexLock(kb_mutex)) {
		if (keyboard_handler())
		{
			dim_timer = timer_get_fixed_seconds();
			if (is_dim)
			{
//				printf("Undimming...\n");
				is_dim = 0;
				VI_Dim(0);
			}
		}
		else if (!is_dim && f2i(timer_get_fixed_seconds() - dim_timer) >= 300)
		{
//			printf("Dimming...\n");
			is_dim = 1;
			VI_Dim(1);
		}
		LWP_MutexUnlock(kb_mutex);
		SYS_SetAlarm(alarm, &tm, (alarmcallback)keyboard_tick, (void*)kb_thread);
		LWP_SuspendThread(kb_thread);
	}

	SYS_RemoveAlarm(alarm);
	VI_Dim(0);

	return NULL;
}

void key_close()
{
	void *x;
	if (!Installed)
		return;
	if (kb_mutex != LWP_MUTEX_NULL) {
		LWP_MutexDestroy(kb_mutex);
		kb_mutex = LWP_MUTEX_NULL;
	}
	if (kb_thread != LWP_THREAD_NULL) {
		LWP_JoinThread(kb_thread, &x);
		kb_thread = LWP_THREAD_NULL;
	}
#ifdef __wii__
	WPAD_Shutdown();
#endif
	Installed = 0;
}

void key_init()
{
	// Initialize queue

	keyd_time_when_last_pressed = timer_get_fixed_seconds();
	keyd_repeat = 1;

	if (Installed) return;
#ifdef __wii__
	Keyboard.exp_type = WPAD_EXP_UNKNOWN;
#else
	Keyboard.exp_type = WPAD_EXP_CLASSIC;
#endif

	LWP_MutexInit(&kb_mutex, 0);

	PAD_Init();
	Joystick.old_buttons = JOY_ALL_BUTTONS;
#ifdef __wii__
	WPAD_Init();
	WPAD_SetVRes(WPAD_CHAN_0, 320, 200);
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
#endif

	Installed = 1;

	mouse_flush();
	joy_flush();
	key_flush();
	LWP_CreateThread(&kb_thread, keyboard_thread, NULL, NULL, 0, KEY_THREAD_PRIO);
	atexit(key_close);
}

unsigned char key_to_ascii(int keycode)
{
	int shifted;

	shifted = keycode & KEY_SHIFTED;
	keycode &= 0xFF;

	if ( keycode>=127 )
		return 255;

	if (shifted)
		return shifted_ascii_table[keycode];
	else
		return ascii_table[keycode];
}

void key_flush()
{
	int i;
	fix curtime;

	if (!Installed)
		key_init();

	LWP_MutexLock(kb_mutex);

	Keyboard.keyhead = Keyboard.keytail = 0;

	//Clear the keyboard buffer
	for (i=0; i<KEY_BUFFER_SIZE; i++ )	{
		Keyboard.keybuffer[i] = 0;
		Keyboard.time_pressed[i] = 0;
	}

	curtime = timer_get_fixed_seconds();

	for (i=0; i<128; i++ )	{
		keyd_pressed[i] = 0;
		Keyboard.keys[i].timewentdown = curtime;
		Keyboard.keys[i].downcount=0;
		Keyboard.keys[i].upcount=0;
		Keyboard.keys[i].timehelddown = 0;
		Keyboard.keys[i].counter = 0;
	}

	LWP_MutexUnlock(kb_mutex);
}

static int add_one( int n )
{
	return (n+1) % KEY_BUFFER_SIZE;
}

int key_inkey()
{
	return key_inkey_time(NULL);
}

int key_inkey_time(fix * time)
{
	int key = 0;
	fix tick = timer_get_fixed_seconds();

	if (!Installed)
		key_init();

	LWP_MutexLock(kb_mutex);

	if (Keyboard.keytail!=Keyboard.keyhead)	{
		key = Keyboard.keybuffer[Keyboard.keyhead];
		if (time)
			*time = Keyboard.time_pressed[Keyboard.keyhead];
		Keyboard.keyhead = add_one(Keyboard.keyhead);
	} else {
		// fake keyboard input using joystick
		keyboard_button_handle(KEY_LEFT, tick, Joystick.buttons & JOY_BUTTON_LEFT);
		keyboard_button_handle(KEY_RIGHT, tick, Joystick.buttons & JOY_BUTTON_RIGHT);
		keyboard_button_handle(KEY_DOWN, tick, Joystick.buttons & JOY_BUTTON_DOWN);
		keyboard_button_handle(KEY_UP, tick, Joystick.buttons & JOY_BUTTON_UP);
	}
	LWP_MutexUnlock(kb_mutex);
	return key;
}

int key_peekkey()
{
	int key = 0;

	if (!Installed)
		key_init();

	LWP_MutexLock(kb_mutex);

	if (Keyboard.keytail!=Keyboard.keyhead)
		key = Keyboard.keybuffer[Keyboard.keyhead];

	LWP_MutexUnlock(kb_mutex);
	return key;
}

unsigned int key_get_shift_status()
{
	unsigned int shift_status = 0;

	LWP_MutexLock(kb_mutex);

	if ( keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT] )
		shift_status |= KEY_SHIFTED;

	if ( keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT] )
		shift_status |= KEY_ALTED;

	if ( keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL] )
		shift_status |= KEY_CTRLED;

#ifndef NDEBUG
	if (keyd_pressed[KEY_DELETE])
		shift_status |=KEY_DEBUGGED;
#endif

	LWP_MutexUnlock(kb_mutex);
	return shift_status;
}

// Returns the number of seconds this key has been down since last call.
fix key_down_time(int scancode)
{
	fix time_down, time;

	if ((scancode<0)|| (scancode>127)) return 0;

	LWP_MutexLock(kb_mutex);
	if (!keyd_pressed[scancode]) {
		time_down = Keyboard.keys[scancode].timehelddown;
		Keyboard.keys[scancode].timehelddown = 0;
	} else {
		time = timer_get_fixed_seconds();
		time_down = time - Keyboard.keys[scancode].timewentdown;
		Keyboard.keys[scancode].timewentdown = time;
	}
	LWP_MutexUnlock(kb_mutex);

	return time_down;
}

// Returns number of times key has went from up to down since last call.
unsigned int key_down_count(int scancode)	{
	int n;

	if ((scancode<0)|| (scancode>127)) return 0;

	LWP_MutexLock(kb_mutex);
	n = Keyboard.keys[scancode].downcount;
	Keyboard.keys[scancode].downcount = 0;
	LWP_MutexUnlock(kb_mutex);

	return n;
}

int joy_init() {
	int type = 0;
	if (!Installed)
		key_init();

	LWP_MutexLock(kb_mutex);
	switch (Keyboard.exp_type) {
		case WPAD_EXP_CLASSIC:
			type = JOY_AS_CLASSIC;
			break;
		case WPAD_EXP_NUNCHUK:
			type = JOY_AS_NUNCHUK;
			break;
		default:
			type = JOY_AS_NONE;
	}
	LWP_MutexUnlock(kb_mutex);

	return type;
}

void joy_set_cen() {
	joystick_read_raw_axis(JOY_ALL_AXIS, Joystick.axis_center);
}

ubyte joystick_read_raw_axis( ubyte mask, int * axis ) {
	ubyte read_masks;
	int i;

	read_masks = 0;
	for (i=0; i<JOY_AXIS_COUNT; i++) {
		if (mask & (1<<i))
			axis[i] = 0;
	}

	if (!Installed) return 0;


	LWP_MutexLock(kb_mutex);
	// only axes 2+3 for wiimote
	if (joy_type==JOY_AS_NONE)
		mask &= JOY_2_X_AXIS|JOY_2_Y_AXIS;

	for (i=0; i<JOY_AXIS_COUNT; i++) {
		if (mask & (1<<i)) {
			axis[i] = Joystick.co_ords[i];
			read_masks |= (1<<i);
		}
	}
	LWP_MutexUnlock(kb_mutex);

	return read_masks;
}

void joy_set_type(ubyte type) {
	if (Installed) {
		LWP_MutexLock(kb_mutex);
		joy_type = type;
		if (type==JOY_AS_CLASSIC) {
			// move pointer offscreen
			Mouse.mouse_x = 320;
			Mouse.mouse_y = 200;
		}
		LWP_MutexUnlock(kb_mutex);
	}
}

void joy_flush() {
	int i;
	// flush button states
	if (Installed) {
		LWP_MutexLock(kb_mutex);
		Joystick.buttons = 0;
		for (i=0; i<JOY_BUTTON_COUNT; i++) {
			Joystick.button_down_count[i] = 0;
			Joystick.button_down_tick[i] = 0;
		}
		LWP_MutexUnlock(kb_mutex);
	}
}

int joy_get_btns()
{
	int buttons=0;
	if (Installed) {
		LWP_MutexLock(kb_mutex);
		buttons = Joystick.buttons;
		LWP_MutexUnlock(kb_mutex);
	}

	return buttons;
}

int joy_get_button_down_cnt( int btn )
{
	int i=0;

	if (Installed && btn>=0 && btn<JOY_BUTTON_COUNT) {
		LWP_MutexLock(kb_mutex);
		i = Joystick.button_down_count[btn];
		Joystick.button_down_count[btn] = 0;
		LWP_MutexUnlock(kb_mutex);
	}

	return i;
}

fix joy_get_button_down_time( int btn )
{
	fix i = 0;

	if (Installed && btn>=0 && btn<JOY_BUTTON_COUNT) {
		LWP_MutexLock(kb_mutex);

		if (Joystick.buttons & (1<<btn)) {
			fix time_now = timer_get_fixed_seconds();
			i = time_now - Joystick.button_down_tick[btn];
			Joystick.button_down_tick[btn] = time_now;
		} else {
			i = Joystick.button_down_tick[btn];
			Joystick.button_down_tick[btn] = 0;
		}

		LWP_MutexUnlock(kb_mutex);
	}

	return i;
}

ubyte joy_get_present_mask()
{
	ubyte mask=0;

	if (Installed) {
		LWP_MutexLock(kb_mutex);
		switch (Keyboard.exp_type) {
			case WPAD_EXP_CLASSIC:
			case WPAD_EXP_NUNCHUK:
				mask |= JOY_1_X_AXIS|JOY_1_Y_AXIS;
			default:
				mask |= JOY_2_X_AXIS|JOY_2_Y_AXIS;
		}
		LWP_MutexUnlock(kb_mutex);
	}

	return mask;
}

int joy_get_button_state( int btn )
{
	int i=0;

	if (Installed) {
		LWP_MutexLock(kb_mutex);
		i = !!(Joystick.buttons & (1<<btn));
		LWP_MutexUnlock(kb_mutex);
	}

	return i;
}

void joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max) {
	int i;

	if (Installed) {
		LWP_MutexLock(kb_mutex);
		for (i=0; i<JOY_AXIS_COUNT; i++)	{
			axis_min[i] = Joystick.axis_min[i];
			axis_center[i] = Joystick.axis_center[i];
			axis_max[i] = Joystick.axis_max[i];
		}
		LWP_MutexUnlock(kb_mutex);
	}
}

void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max) {
	int i;

	if (Installed && joy_type!=JOY_AS_NONE) {
		LWP_MutexLock(kb_mutex);
		for (i=0; i<JOY_AXIS_COUNT; i++) {
			Joystick.axis_min[i] = axis_min[i];
			Joystick.axis_center[i] = axis_center[i];
			Joystick.axis_max[i] = axis_max[i];
			if (joy_type!=JOY_AS_CLASSIC && i==1)
				break;
		}
		Joystick.wrote_cal = 5;
		LWP_MutexUnlock(kb_mutex);
	}

}

int joy_get_scaled_reading( int raw, int axn ) {
	int x, d;

	if (!Installed) return 0;

	LWP_MutexLock(kb_mutex);

	if (raw < Joystick.axis_min[axn])
		raw = Joystick.axis_min[axn];
	else if (raw > Joystick.axis_max[axn])
		raw = Joystick.axis_max[axn];

	raw -= Joystick.axis_center[axn];

	if (raw < 0)
		d = Joystick.axis_center[axn]-Joystick.axis_min[axn];
	else
		d = Joystick.axis_max[axn]-Joystick.axis_center[axn];

	// Make sure it's calibrated properly.
	if (Joystick.axis_center[axn] - Joystick.axis_min[axn] < 5) return 0;
	if (Joystick.axis_max[axn] - Joystick.axis_center[axn] < 5) return 0;

	LWP_MutexUnlock(kb_mutex);

	if (d)
		x = (raw << 7) / d;
	else
		return 0;

	x = MAX(-128, MIN(127, x));

	return x;
}

int mouse_init() {
	if (!Installed)
		key_init();

	return 1;
}

void mouse_flush()
{
	int i;

	if (!Installed)
		return;

	LWP_MutexLock(kb_mutex);
	for (i = 0; i < MOUSE_MAX_BUTTONS; i++) {
		Mouse.pressed[i] = 0;
		Mouse.time_held_down[i] = 0;
		Mouse.num_downs[i] = 0;
		Mouse.num_ups[i] = 0;
		Mouse.went_down[i] = 0;
	}
	LWP_MutexUnlock(kb_mutex);
}

void mouse_get_pos( int *x, int *y) {
	if (!Installed)
		return;

	LWP_MutexLock(kb_mutex);
	*x = Mouse.mouse_x;
	*y = Mouse.mouse_y;
	LWP_MutexUnlock(kb_mutex);
}

void mouse_get_delta( int *dx, int *dy ) {
	*dx = *dy = 0;
}

int mouse_get_btns() {
	int i;
	uint flag=1;
	int status = 0;

	if (!Installed)
		return 0;

	LWP_MutexLock(kb_mutex);
	for (i = 0; i < MOUSE_MAX_BUTTONS; i++) {
		if (Mouse.pressed[i])
			status |= flag;
		flag <<= 1;
	}
	LWP_MutexUnlock(kb_mutex);

	return status;
}

fix mouse_button_down_time(int button) {
	fix time_down=0;

	if (Installed && button>=0 && button<MOUSE_MAX_BUTTONS) {
		LWP_MutexLock(kb_mutex);
		// if it's currently pressed, time_held_down is the time it went down
		if (!Mouse.pressed[button]) {
			time_down = Mouse.time_held_down[button];
			Mouse.time_held_down[button] = 0;
		} else {
			fix time_now = timer_get_fixed_seconds();
			time_down = time_now - Mouse.time_held_down[button];
			Mouse.time_held_down[button] = time_now;
		}
		LWP_MutexUnlock(kb_mutex);
	}

	return time_down;
}

int mouse_button_down_count(int button) {
	int count=0;

	if (Installed && button>=0 && button<MOUSE_MAX_BUTTONS) {
		LWP_MutexLock(kb_mutex);
		count = Mouse.num_downs[button];
		Mouse.num_downs[button] = 0;
		LWP_MutexUnlock(kb_mutex);
	}

	return count;
}

int mouse_button_state(int button) {
	int state=0;

	if (Installed && button>=0 && button<MOUSE_MAX_BUTTONS) {
		LWP_MutexLock(kb_mutex);
		state = Mouse.pressed[button];
		LWP_MutexUnlock(kb_mutex);
	}

	return state;
}

int mouse_went_down(int button) {
	int state=0;

	if (Installed && button>=0 && button<MOUSE_MAX_BUTTONS) {
		LWP_MutexLock(kb_mutex);
		state = Mouse.went_down[button];
		Mouse.went_down[button] = 0;
		LWP_MutexUnlock(kb_mutex);
	}

	return state;
}


