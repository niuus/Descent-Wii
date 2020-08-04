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

#ifdef NETWORK

#ifndef _MULTIBOT_H
#define _MULTIBOT_H

#ifndef SHAREWARE

#include "object.h"

#define MAX_ROBOTS_CONTROLLED 3

#define ROBOT_FIRE_AGITATION 94

extern int robot_controlled[MAX_ROBOTS_CONTROLLED];
extern int robot_agitation[MAX_ROBOTS_CONTROLLED];
extern int robot_fired[MAX_ROBOTS_CONTROLLED];

int multi_can_move_robot(int objnum, int agitation);
void multi_send_robot_position(int objnum, int fired);
void multi_send_robot_fire(int objnum, int gun_num, vms_vector *fire);
void multi_send_claim_robot(int objnum);
void multi_send_robot_explode(int objnum, int killer);
void multi_send_create_robot(int robotcen, int objnum, int type);
void multi_send_boss_actions(int bossobjnum, int action, int secondary, int objnum);
int multi_send_robot_frame(int sent);

void multi_do_robot_explode(unsigned char *buf);
void multi_do_robot_position(unsigned char *buf);
void multi_do_claim_robot(unsigned char *buf);
void multi_do_release_robot(unsigned char *buf);
void multi_do_robot_fire(unsigned char *buf);
void multi_do_create_robot(unsigned char *buf);
void multi_do_boss_actions(unsigned char *buf);
void multi_do_create_robot_powerups(unsigned char *buf);

int multi_explode_robot_sub(int botnum, int killer);

void multi_drop_robot_powerups(int objnum);
void multi_dump_robots(void);

void multi_strip_robots(int playernum);
void multi_check_robot_timeout(void);

void multi_robot_request_change(object *robot, int playernum);
#endif
#endif
#endif