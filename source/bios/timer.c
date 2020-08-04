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

#include "dtypes.h"
#include "gtimer.h"

#include <gccore.h>
#include <ogcsys.h>

#define COUNTER_RATE	128
#define COUNTER_TIME	(1e9/COUNTER_RATE)

static int Installed = 0;
static volatile uint timer_count;

static lwpq_t timer_queue=LWP_TQUEUE_NULL;
static syswd_t timer=0;

fix timer_get_approx_seconds()
{
	return i2f(timer_count / COUNTER_RATE);
}

fix timer_get_fixed_seconds()
{
	return fixmuldiv(timer_count, F1_0, COUNTER_RATE);
}

void delay(int d_time)
{
	fix start, delay_time;

	if (d_time<=0) return;

	start = timer_get_fixed_seconds();
	delay_time = fixmuldiv(d_time, F1_0, 1000);

	// this calculation is designed to account for timer overflow
	while ((timer_get_fixed_seconds() - delay_time) < start) {
		// pause thread to free up CPU for other tasks (music, input)
		if (timer_queue != LWP_TQUEUE_NULL)
			LWP_ThreadSleep(timer_queue);
	}
}

void counter_timer(syswd_t alarm, void *x)
{
	timer_count++;
	LWP_ThreadBroadcast(timer_queue);
}

void timer_close()
{
	if (!Installed)
		return;

	if (timer)
		SYS_RemoveAlarm(timer);
	timer = 0;

	if (timer_queue != LWP_TQUEUE_NULL)
		LWP_CloseQueue(timer_queue);
	timer_queue = LWP_TQUEUE_NULL;

	Installed = 0;
}

void timer_init()
{
	if (Installed)
		return;

	Installed = 1;

	SYS_CreateAlarm(&timer);
	timer_count = 0;
	LWP_InitQueue(&timer_queue);
	struct timespec tm = {0, COUNTER_TIME};
	SYS_SetPeriodicAlarm(timer, &tm, &tm, counter_timer, NULL);

	atexit(timer_close);
}
