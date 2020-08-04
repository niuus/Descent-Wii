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

#ifndef _ROBOT_H
#define _ROBOT_H

#include "vecmat.h"
#include "object.h"
#include "game.h"

#define MAX_GUNS 8		//should be multiple of 4 for ubyte array

//Animation states
#define AS_REST			0
#define AS_ALERT			1
#define AS_FIRE			2
#define AS_RECOIL			3
#define AS_FLINCH			4
#define N_ANIM_STATES	5

#define	RI_CLOAKED_NEVER					0
#define	RI_CLOAKED_ALWAYS					1
#define	RI_CLOAKED_EXCEPT_FIRING		2

//describes the position of a certain joint
typedef struct jointpos {
	short jointnum;
	vms_angvec angles;
} jointpos;

//describes a list of joint positions
typedef struct jointlist {
	short n_joints;
	short offset;
} jointlist;

//	Robot information
typedef struct robot_info {
	int			model_num;							// which polygon model?
	int			n_guns;								// how many different gun positions
	vms_vector	gun_points[MAX_GUNS];			// where each gun model is
	ubyte			gun_submodels[MAX_GUNS];		// which submodel is each gun in?
	short 		exp1_vclip_num;
	short			exp1_sound_num;
	short 		exp2_vclip_num;
	short			exp2_sound_num;
	short			weapon_type;
	byte			contains_id;						//	ID of powerup this robot can contain.
	byte			contains_count;					//	Max number of things this instance can contain.
	byte			contains_prob;						//	Probability that this instance will contain something in N/16
	byte			contains_type;						//	Type of thing contained, robot or powerup, in bitmaps.tbl, !0=robot, 0=powerup
	int			score_value;						//	Score from this robot.
	fix			lighting;							// should this be here or with polygon model?
	fix			strength;							// Initial shields of robot

	fix		mass;										// how heavy is this thing?
	fix		drag;										// how much drag does it have?

	fix		field_of_view[NDL];					// compare this value with forward_vector.dot.vector_to_player, if field_of_view <, then robot can see player
	fix		firing_wait[NDL];						//	time in seconds between shots
	fix		turn_time[NDL];						// time in seconds to rotate 360 degrees in a dimension
	fix		fire_power[NDL];						//	damage done by a hit from this robot
	fix		shield[NDL];							//	shield strength of this robot
	fix		max_speed[NDL];						//	maximum speed attainable by this robot
	fix		circle_distance[NDL];				//	distance at which robot circles player

	byte		rapidfire_count[NDL];				//	number of shots fired rapidly
	byte		evade_speed[NDL];						//	rate at which robot can evade shots, 0=none, 4=very fast
	byte		cloak_type;								//	0=never, 1=always, 2=except-when-firing
	byte		attack_type;							//	0=firing, 1=charge (like green guy)
	byte		boss_flag;								//	0 = not boss, 1 = boss.  Is that surprising?
	ubyte		see_sound;								//	sound robot makes when it first sees the player
	ubyte		attack_sound;							//	sound robot makes when it attacks the player
	ubyte		claw_sound;								//	sound robot makes as it claws you (attack_type should be 1)

	//animation info
	jointlist anim_states[MAX_GUNS+1][N_ANIM_STATES];

	int		always_0xabcd;							// debugging

} robot_info;


#define	MAX_ROBOT_TYPES	30				// maximum number of robot types

#define	ROBOT_NAME_LENGTH	16
extern char	Robot_names[MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];

//the array of robots types
extern robot_info Robot_info[];			// Robot info for AI system, loaded from bitmaps.tbl.

//how many kinds of robots
extern	int	N_robot_types;		// Number of robot types.  We used to assume this was the same as N_polygon_models.

//test data for one robot
#define MAX_ROBOT_JOINTS 600
extern jointpos Robot_joints[MAX_ROBOT_JOINTS];
extern int	N_robot_joints;

//given an object and a gun number, return position in 3-space of gun
//fills in gun_point
void calc_gun_point(vms_vector *gun_point,object *obj,int gun_num);
//void calc_gun_point(vms_vector *gun_point,int objnum,int gun_num);

//	Tells joint positions for a gun to be in a specified state.
//	A gun can have associated with it any number of joints.  In order to tell whether a gun is a certain
//	state (such as FIRE or ALERT), you should call this function and check the returned joint positions
//	against the robot's gun's joint positions.  This function should also be called to determine how to
//	move a gun into a desired position.
//	For now (May 30, 1994), it is assumed that guns will linearly interpolate from one joint position to another.
//	There is no ordering of joint movement, so it's impossible to guarantee that a strange starting position won't
//	cause a gun to move through a robot's body, for example.

//	Given:
//		jp_list_ptr		pointer to list of joint angles, on exit, this is pointing at a static array
//		robot_type		type of robot for which to get joint information.  A particular type, not an instance of a robot.
//		gun_num			gun number.  If in 0..Robot_info[robot_type].n_guns-1, then it is a gun, else it refers to non-animating parts of robot.
//		state				state about which to get information.  Legal states in range 0..N_ANIM_STATES-1, defined in robot.h, are:
//								AS_REST, AS_ALERT, AS_FIRE, AS_RECOIL, AS_FLINCH

//	On exit:
//		Returns number of joints in list.
//		jp_list_ptr is stuffed with a pointer to a static array of joint positions.  This pointer is valid forever.
extern int robot_get_anim_state(jointpos **jp_list_ptr,int robot_type,int gun_num,int state);

#endif
