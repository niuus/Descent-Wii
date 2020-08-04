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

#include "settings.h"
#ifdef NETWORK

#include <string.h>

#include <stdio.h>

#include "macnet.h"
#include "ipx.h"
#include "byteswap.h"
#include "multi.h"
#include "network.h"
#include "object.h"
#include "powerup.h"
#include "error.h"

// __lhbrx,__lwbrx,__sthbrx,__stwbrx
#include "ogc/machine/processor.h"

extern ubyte object_buffer[IPX_MAX_DATA_SIZE];		// used for tmp netgame packets as well as sending object data
extern frame_info 	MySyncPack;

void receive_netplayer_info(ubyte *data, netplayer_info *info)
{
	int loc = 0;

	memcpy(info->callsign, data+loc, CALLSIGN_LEN+1);		loc += CALLSIGN_LEN+1;
	memcpy(info->network_info.ipx_info.server, data+loc, 4);					loc += 4;
	memcpy(info->network_info.ipx_info.node, data+loc, 6);						loc += 6;
	memcpy(&(info->network_info.ipx_info.socket), data+loc, 2);					loc += 2;
	info->connected = data[loc];
}

void send_sequence_packet(sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address)
{
	u32 loc=0;

	memset(object_buffer, 0, sizeof(object_buffer));
	object_buffer[0] = seq.type;			loc++;
	memcpy(object_buffer+loc, seq.player.callsign, CALLSIGN_LEN+1);		loc += CALLSIGN_LEN+1;
	memcpy(object_buffer+loc, seq.player.network_info.ipx_info.server, 4);		loc += 4;
	memcpy(object_buffer+loc, seq.player.network_info.ipx_info.node, 6);		loc += 6;
	memcpy(object_buffer+loc, &seq.player.network_info.ipx_info.socket, 2);    loc += 2;
	object_buffer[loc++] = seq.player.connected;

	if (net_address != NULL)
		ipx_send_packet_data( object_buffer, loc, server, node, net_address);
	else if ((server == NULL) && (node == NULL))
		ipx_send_broadcast_packet_data( object_buffer, loc );
	else
		ipx_send_internetwork_packet_data( object_buffer, loc, server, node);
}

void receive_sequence_packet(ubyte *data, sequence_packet *seq)
{
	seq->type = data[0];
	receive_netplayer_info(data+1, &(seq->player));
}


void send_netgame_packet(ubyte *server, ubyte *node)
{
	int i, j;
	u32 loc = 0;

	memset(object_buffer, 0, sizeof(object_buffer));
	object_buffer[loc++] = Netgame.type;
	memcpy(object_buffer+loc, Netgame.game_name, NETGAME_NAME_LEN+1); loc += (NETGAME_NAME_LEN+1);
	memcpy(object_buffer+loc, Netgame.team_name, 2*(CALLSIGN_LEN+1)); loc += 2*(CALLSIGN_LEN+1);
	object_buffer[loc++] = Netgame.gamemode;
	object_buffer[loc++] = Netgame.difficulty;
	object_buffer[loc++] = Netgame.game_status;
	object_buffer[loc++] = Netgame.numplayers;
	object_buffer[loc++] = Netgame.max_numplayers;
	object_buffer[loc++] = Netgame.game_flags;
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(object_buffer+loc, Netgame.players[i].callsign, CALLSIGN_LEN+1);	loc += CALLSIGN_LEN+1;
		memcpy(object_buffer+loc, Netgame.players[i].network_info.ipx_info.server, 4);					loc += 4;
		memcpy(object_buffer+loc, Netgame.players[i].network_info.ipx_info.node, 6);						loc += 6;
		memcpy(object_buffer+loc, &(Netgame.players[i].network_info.ipx_info.socket), 2);				loc += 2;
		object_buffer[loc++] = Netgame.players[i].connected;
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		__stwbrx(object_buffer, loc, Netgame.locations[i]);            loc += 4;
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			__sthbrx(object_buffer, loc, Netgame.kills[i][j]);         loc += 2;
		}
	}

	__stwbrx(object_buffer, loc, Netgame.levelnum);                    loc += 4;
	object_buffer[loc++] = Netgame.protocol_version;
	object_buffer[loc++] = Netgame.team_vector;
	__sthbrx(object_buffer, loc, Netgame.segments_checksum);           loc += 2;
	__sthbrx(object_buffer, loc, Netgame.team_kills[0]);               loc += 2;
	__sthbrx(object_buffer, loc, Netgame.team_kills[1]);               loc += 2;

	for (i = 0; i < MAX_PLAYERS; i++) {
		__sthbrx(object_buffer, loc, Netgame.killed[i]);               loc += 2;
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		__sthbrx(object_buffer, loc, Netgame.player_kills[i]);         loc += 2;
	}

#ifndef MAC_SHAREWARE
	__stwbrx(object_buffer, loc, Netgame.level_time);                  loc += 4;
	__stwbrx(object_buffer, loc, Netgame.control_invul_time);          loc += 4;
	__stwbrx(object_buffer, loc, Netgame.monitor_vector);              loc += 4;

	for (i = 0; i < 8; i++) {
		__stwbrx(object_buffer, loc, Netgame.player_score[i]);         loc += 4;
	}

	memcpy(object_buffer+loc, Netgame.player_flags, 8);                loc += 8;
	memcpy(object_buffer+loc, Netgame.mission_name, 9);                loc += 9;
	memcpy(object_buffer+loc, Netgame.mission_title, MISSION_NAME_LEN+1); loc += (MISSION_NAME_LEN+1);
#endif

	if (server == NULL)
		ipx_send_broadcast_packet_data(object_buffer, loc);
	else
		ipx_send_internetwork_packet_data( object_buffer, loc, server, node );
}

void receive_netgame_packet(ubyte *data, netgame_info *netgame)
{
	int i, j;
	u32 loc = 0;

	netgame->type = data[loc++];
	memcpy(netgame->game_name, data+loc, NETGAME_NAME_LEN+1); loc += (NETGAME_NAME_LEN+1);
	memcpy(netgame->team_name, data+loc, 2*(CALLSIGN_LEN+1)); loc += 2*(CALLSIGN_LEN+1);
	netgame->gamemode = data[loc++];
	netgame->difficulty = data[loc++];
	netgame->game_status = data[loc++];
	netgame->numplayers = data[loc++];
	netgame->max_numplayers = data[loc++];
	netgame->game_flags = data[loc++];
	for (i = 0; i < MAX_PLAYERS; i++) {
		receive_netplayer_info(data+loc, netgame->players+i);
		loc += 22;
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		netgame->locations[i] = __lwbrx(data, loc);        loc += 4;
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			netgame->kills[i][j] = __lhbrx(data, loc);     loc += 2;
		}
	}

	netgame->levelnum = __lwbrx(data, loc);                loc += 4;
	netgame->protocol_version = data[loc++];
	netgame->team_vector = data[loc++];
	netgame->segments_checksum = __lhbrx(data, loc);       loc += 2;
	netgame->team_kills[0] = __lhbrx(data, loc);           loc += 2;
	netgame->team_kills[1] = __lhbrx(data, loc);           loc += 2;
	for (i = 0; i < MAX_PLAYERS; i++) {
		netgame->killed[i] = __lhbrx(data, loc);           loc += 2;
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		netgame->player_kills[i] = __lhbrx(data, loc);     loc += 2;
	}

#ifndef MAC_SHAREWARE
	netgame->level_time = __lwbrx(data, loc);              loc += 4;
	netgame->control_invul_time = __lwbrx(data, loc);      loc += 4;
	netgame->monitor_vector = __lwbrx(data, loc);          loc += 4;
	for (i = 0; i < 8; i++) {
		netgame->player_score[i] = __lwbrx(data, loc);     loc += 4;
	}

	memcpy(netgame->player_flags, data+loc, 8);            loc += 8;
	memcpy(netgame->mission_name, data+loc, 9);            loc += 9;
	memcpy(netgame->mission_title,data+loc, MISSION_NAME_LEN+1); loc += (MISSION_NAME_LEN+1);
#endif
}

void send_frameinfo_packet(ubyte *server, ubyte *node, ubyte *address)
{
	u32 loc = 0;

	memset(object_buffer, 0, sizeof(object_buffer));

#ifdef MAC_SHAREWARE // may contain bugs, has never been tested
	object_buffer[loc++] = MySyncPack.type;
	__stwbrx(object_buffer, loc, MySyncPack.numpackets);            loc += 4;
	__sthbrx(object_buffer, loc, MySyncPack.objnum);                loc += 2;
	object_buffer[loc++] = MySyncPack.playernum;
	__sthbrx(object_buffer, loc, MySyncPack.obj_segnum);            loc += 2;

	__stwbrx(object_buffer, loc, MySyncPack.obj_pos.x);             loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_pos.y);             loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_pos.z);             loc += 4;

	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.rvec.x);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.rvec.y);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.rvec.z);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.uvec.x);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.uvec.y);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.uvec.z);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.fvec.x);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.fvec.y);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.fvec.z);     loc += 4;

	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.velocity.x); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.velocity.y); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.velocity.z); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.thrust.x); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.thrust.y); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.thrust.z); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.mass);    loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.drag);    loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.brakes);  loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.rotvel.x); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.rotvel.y); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.rotvel.z); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.rotthrust.x); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.rotthrust.y); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_phys_info.rotthrust.z); loc += 4;
	__sthbrx(object_buffer, loc, MySyncPack.obj_phys_info.turnroll); loc += 2;
	__sthbrx(object_buffer, loc, MySyncPack.obj_phys_info.flags);    loc += 2;

	object_buffer[loc+] = MySyncPack.obj_render_type;
	object_buffer[loc+] = MySyncPack.level_num;
	__sthbrx(object_buffer, loc, MySyncPack.data_size);              loc += 2;
	memcpy(object_buffer+loc, MySyncPack.data, MySyncPack.data_size);
	loc += MySyncPack.data_size;
#else
	object_buffer[0] = MySyncPack.type;		loc++;	  loc += 3;		// skip three for pad byte
	__stwbrx(object_buffer, loc, MySyncPack.numpackets); loc += 4;

	__stwbrx(object_buffer, loc, MySyncPack.obj_pos.x);  loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_pos.y);  loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_pos.z);  loc += 4;

	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.rvec.x); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.rvec.y); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.rvec.z); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.uvec.x); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.uvec.y); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.uvec.z); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.fvec.x); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.fvec.y); loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.obj_orient.fvec.z); loc += 4;

	__stwbrx(object_buffer, loc, MySyncPack.phys_velocity.x);   loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.phys_velocity.y);   loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.phys_velocity.z);   loc += 4;

	__stwbrx(object_buffer, loc, MySyncPack.phys_rotvel.x);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.phys_rotvel.y);     loc += 4;
	__stwbrx(object_buffer, loc, MySyncPack.phys_rotvel.z);     loc += 4;

	__sthbrx(object_buffer, loc, MySyncPack.obj_segnum);        loc += 2;
	__sthbrx(object_buffer, loc, MySyncPack.data_size);         loc += 2;

	object_buffer[loc++] = MySyncPack.playernum;
	object_buffer[loc++] = MySyncPack.obj_render_type;
	object_buffer[loc++] = MySyncPack.level_num;
	memcpy(object_buffer+loc, MySyncPack.data, MySyncPack.data_size);
	loc += MySyncPack.data_size;
#endif

	if (address != NULL)
		ipx_send_packet_data( object_buffer, loc, server, node, address);
	else
		ipx_send_internetwork_packet_data( object_buffer, loc, server, node );
}

void receive_frameinfo_packet(ubyte *data, frame_info *info)
{
	u32 loc = 0;

#ifdef MAC_SHAREWARE // untested
	info->type = data[loc++];
	info->numpackets = __lwbrx(data, loc);                   loc += 4;
	info->objnum = __lhbrx(data, loc);                       loc += 2;
	info->playernum = data[loc++];
	info->obj_segnum = __lhbrx(data, loc);                   loc += 2;

	info->obj_pos.x = __lwbrx(data, loc);                    loc += 4;
	info->obj_pos.y = __lwbrx(data, loc);                    loc += 4;
	info->obj_pos.z = __lwbrx(data, loc);                    loc += 4;

	info->obj_orient.rvec.x = __lwbrx(data, loc);            loc += 4;
	info->obj_orient.rvec.y = __lwbrx(data, loc);            loc += 4;
	info->obj_orient.rvec.z = __lwbrx(data, loc);            loc += 4;
	info->obj_orient.uvec.x = __lwbrx(data, loc);            loc += 4;
	info->obj_orient.uvec.y = __lwbrx(data, loc);            loc += 4;
	info->obj_orient.uvec.z = __lwbrx(data, loc);            loc += 4;
	info->obj_orient.fvec.x = __lwbrx(data, loc);            loc += 4;
	info->obj_orient.fvec.y = __lwbrx(data, loc);            loc += 4;
	info->obj_orient.fvec.z = __lwbrx(data, loc);            loc += 4;

	info->obj_phys_info.velocity.x = __lwbrx(data, loc);     loc += 4;
	info->obj_phys_info.velocity.y = __lwbrx(data, loc);     loc += 4;
	info->obj_phys_info.velocity.z = __lwbrx(data, loc);     loc += 4;

	info->obj_phys_info.thrust.x = __lwbrx(data, loc);       loc += 4;
	info->obj_phys_info.thrust.y = __lwbrx(data, loc);       loc += 4;
	info->obj_phys_info.thrust.z = __lwbrx(data, loc);       loc += 4;

	info->obj_phys_info.mass = __lwbrx(data, loc);           loc += 4;
	info->obj_phys_info.drag = __lwbrx(data, loc);           loc += 4;
	info->obj_phys_info.brakes = __lwbrx(data, loc);         loc += 4;

	info->obj_phys_info.rotvel.x = __lwbrx(data, loc);       loc += 4;
	info->obj_phys_info.rotvel.y = __lwbrx(data, loc);       loc += 4;
	info->obj_phys_info.rotvel.z = __lwbrx(data, loc);       loc += 4;

	info->obj_phys_info.rotthrust.x = __lwbrx(data, loc);    loc += 4;
	info->obj_phys_info.rotthrust.y = __lwbrx(data, loc);    loc += 4;
	info->obj_phys_info.rotthrust.z = __lwbrx(data, loc);    loc += 4;

	info->obj_phys_info.turnroll = __lhbrx(data, loc);       loc += 2;
	info->obj_phys_info.flags = __lhbrx(data, loc);          loc += 2;
	info->obj_render_type = data[loc++];
	info->level_num = data[loc++];
	info->data_size = __lhbrx(data, loc);                    loc += 2;
	memcpy(info->data, data+loc, info->data_size);
#else
	info->type = data[0];			loc++;  loc += 3;		// skip three for pad byte
	info->numpackets = __lwbrx(data, loc);                  loc += 4;

	info->obj_pos.x = __lwbrx(data, loc);                   loc += 4;
	info->obj_pos.y = __lwbrx(data, loc);                   loc += 4;
	info->obj_pos.z = __lwbrx(data, loc);                   loc += 4;

	info->obj_orient.rvec.x = __lwbrx(data, loc);           loc += 4;
	info->obj_orient.rvec.y = __lwbrx(data, loc);           loc += 4;
	info->obj_orient.rvec.z = __lwbrx(data, loc);           loc += 4;
	info->obj_orient.uvec.x = __lwbrx(data, loc);           loc += 4;
	info->obj_orient.uvec.y = __lwbrx(data, loc);           loc += 4;
	info->obj_orient.uvec.z = __lwbrx(data, loc);           loc += 4;
	info->obj_orient.fvec.x = __lwbrx(data, loc);           loc += 4;
	info->obj_orient.fvec.y = __lwbrx(data, loc);           loc += 4;
	info->obj_orient.fvec.z = __lwbrx(data, loc);           loc += 4;

	info->phys_velocity.x = __lwbrx(data, loc);             loc += 4;
	info->phys_velocity.y = __lwbrx(data, loc);             loc += 4;
	info->phys_velocity.z = __lwbrx(data, loc);             loc += 4;

	info->phys_rotvel.x = __lwbrx(data, loc);               loc += 4;
	info->phys_rotvel.y = __lwbrx(data, loc);               loc += 4;
	info->phys_rotvel.z = __lwbrx(data, loc);               loc += 4;

	info->obj_segnum = __lhbrx(data, loc);                  loc += 2;
	info->data_size = __lhbrx(data, loc);                   loc += 2;

	info->playernum = data[loc++];
	info->obj_render_type = data[loc++];
	info->level_num = data[loc++];
	memcpy(info->data, data+loc, info->data_size);
#endif
}

void send_endlevel_packet(endlevel_info *info, ubyte *server, ubyte *node, ubyte *address)
{
	int i, j;
	u32 loc = 0;

	memset(object_buffer, 0, sizeof(object_buffer));
	object_buffer[loc++] = info->type;
	object_buffer[loc++] = info->player_num;
	object_buffer[loc++] = info->connected;
	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			__sthbrx(object_buffer, loc, info->kill_matrix[i][j]);   loc += 2;
		}
	}
	__sthbrx(object_buffer, loc, info->kills);                       loc += 2;
	__sthbrx(object_buffer, loc, info->killed);                      loc += 2;
	object_buffer[loc++] = info->seconds_left;

	ipx_send_packet_data( object_buffer, loc, server, node, address);
}

void receive_endlevel_packet(ubyte *data, endlevel_info *info)
{
	int i, j;
	u32 loc = 0;

	info->type = data[loc++];
	info->player_num = data[loc++];
	info->connected = data[loc++];

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			info->kill_matrix[i][j] = __lhbrx(data, loc);         loc += 2;
		}
	}
	info->kills = __lhbrx(data, loc);                             loc += 2;
	info->killed = __lhbrx(data, loc);                            loc += 2;
	info->seconds_left = data[loc];
}

void pack_object(ubyte *dest, object *src) {
	u32 loc=0;

	__stwbrx(dest, loc, src->signature);                                loc += 4;
	dest[loc++] = src->type;
	dest[loc++] = src->id;
	__sthbrx(dest, loc, src->next);                                     loc += 2;
	__sthbrx(dest, loc, src->prev);                                     loc += 2;
	dest[loc++] = src->control_type;
	dest[loc++] = src->movement_type;
	dest[loc++] = src->render_type;
	dest[loc++] = src->flags;
	__sthbrx(dest, loc, src->segnum);                                   loc += 2;
	__sthbrx(dest, loc, src->attached_obj);                             loc += 2;

	__stwbrx(dest, loc, src->pos.x);                                    loc += 4;
	__stwbrx(dest, loc, src->pos.y);                                    loc += 4;
	__stwbrx(dest, loc, src->pos.z);                                    loc += 4;

	__stwbrx(dest, loc, src->orient.rvec.x);                            loc += 4;
	__stwbrx(dest, loc, src->orient.rvec.y);                            loc += 4;
	__stwbrx(dest, loc, src->orient.rvec.z);                            loc += 4;
	__stwbrx(dest, loc, src->orient.fvec.x);                            loc += 4;
	__stwbrx(dest, loc, src->orient.fvec.y);                            loc += 4;
	__stwbrx(dest, loc, src->orient.fvec.z);                            loc += 4;
	__stwbrx(dest, loc, src->orient.uvec.x);                            loc += 4;
	__stwbrx(dest, loc, src->orient.uvec.y);                            loc += 4;
	__stwbrx(dest, loc, src->orient.uvec.z);                            loc += 4;

	__stwbrx(dest, loc, src->size);                                     loc += 4;
	__stwbrx(dest, loc, src->shields);                                  loc += 4;

	__stwbrx(dest, loc, src->last_pos.x);                               loc += 4;
	__stwbrx(dest, loc, src->last_pos.y);                               loc += 4;
	__stwbrx(dest, loc, src->last_pos.z);                               loc += 4;

	dest[loc++] = src->contains_type;
	dest[loc++] = src->contains_id;
	dest[loc++] = src->contains_count;
	dest[loc++] = src->matcen_creator;

	__stwbrx(dest, loc, src->lifeleft);                                 loc += 4;

	// mtype always holds a vms_vector
	__stwbrx(dest, loc, src->mtype.spin_rate.x);                        loc += 4;
	__stwbrx(dest, loc, src->mtype.spin_rate.y);                        loc += 4;
	__stwbrx(dest, loc, src->mtype.spin_rate.z);                        loc += 4;
	// optionally handle the rest of physics_info
	if (src->movement_type==MT_PHYSICS) {
		__stwbrx(dest, loc, src->mtype.phys_info.thrust.x);             loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.thrust.y);             loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.thrust.z);             loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.mass);                 loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.drag);                 loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.brakes);               loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.rotvel.x);             loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.rotvel.y);             loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.rotvel.z);             loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.rotthrust.x);          loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.rotthrust.y);          loc += 4;
		__stwbrx(dest, loc, src->mtype.phys_info.rotthrust.z);          loc += 4;
		__sthbrx(dest, loc, src->mtype.phys_info.turnroll);             loc += 2;
		__sthbrx(dest, loc, src->mtype.phys_info.flags);                loc += 2;
	} else
		loc += 52;

	switch (src->control_type) {

	case CT_WEAPON: // laser_info,laser_info
		__sthbrx(dest, loc, src->ctype.laser_info.parent_type);         loc += 2;
		__sthbrx(dest, loc, src->ctype.laser_info.parent_num);          loc += 2;
		__stwbrx(dest, loc, src->ctype.laser_info.parent_signature);    loc += 4;
		__stwbrx(dest, loc, src->ctype.laser_info.creation_time);       loc += 4;
		__sthbrx(dest, loc, src->ctype.laser_info.last_hitobj);         loc += 2;
		__sthbrx(dest, loc, src->ctype.laser_info.track_goal);          loc += 2;
		__stwbrx(dest, loc, src->ctype.laser_info.multiplier);          loc += 14;
		break;

	case CT_EXPLOSION: // explosion_info,expl_info
		__stwbrx(dest, loc, src->ctype.expl_info.spawn_time);           loc += 4;
		__stwbrx(dest, loc, src->ctype.expl_info.delete_time);          loc += 4;
		__sthbrx(dest, loc, src->ctype.expl_info.delete_objnum);        loc += 2;
		__sthbrx(dest, loc, src->ctype.expl_info.attach_parent);        loc += 2;
		__sthbrx(dest, loc, src->ctype.expl_info.prev_attach);          loc += 2;
		__sthbrx(dest, loc, src->ctype.expl_info.next_attach);          loc += 16;
		break;

	case CT_AI: // ai_static,ai_info
		dest[loc++] = src->ctype.ai_info.behavior;
		memcpy(dest+loc, src->ctype.ai_info.flags, 11/*MAX_AI_FLAGS*/); loc += 11;
		__sthbrx(dest, loc, src->ctype.ai_info.hide_segment);           loc += 2;
		__sthbrx(dest, loc, src->ctype.ai_info.hide_index);             loc += 2;
		__sthbrx(dest, loc, src->ctype.ai_info.path_length);            loc += 2;
		__sthbrx(dest, loc, src->ctype.ai_info.cur_path_index);         loc += 2;
		__sthbrx(dest, loc, src->ctype.ai_info.follow_path_start_seg);  loc += 2;
		__sthbrx(dest, loc, src->ctype.ai_info.follow_path_end_seg);    loc += 2;
		__stwbrx(dest, loc, src->ctype.ai_info.danger_laser_signature); loc += 4;
		__sthbrx(dest, loc, src->ctype.ai_info.danger_laser_num);       loc += 2;
		break;

	case CT_POWERUP: // powerup_info,powerup_info
		// vulcan cannon now comes with included ammo! But why is this special cased here?
		if (src->id == POW_VULCAN_WEAPON)
			src->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
	case CT_LIGHT: // light_info,light_info
		__stwbrx(dest, loc, src->ctype.powerup_info.count);
	default:
		                                                                loc += 30;
	}

	// rtype always starts with an int
	__stwbrx(dest, loc, src->rtype.vclip_info.vclip_num);               loc += 4;
	if (src->render_type==RT_MORPH || src->render_type==RT_POLYOBJ) {
		int i;
		for (i=0; i < 10/*MAX_SUBMODELS*/; i++) {
			__sthbrx(dest, loc, src->rtype.pobj_info.anim_angles[i].p); loc += 2;
			__sthbrx(dest, loc, src->rtype.pobj_info.anim_angles[i].b); loc += 2;
			__sthbrx(dest, loc, src->rtype.pobj_info.anim_angles[i].h); loc += 2;
		}
		__stwbrx(dest, loc, src->rtype.pobj_info.subobj_flags);         loc += 4;
		__stwbrx(dest, loc, src->rtype.pobj_info.tmap_override);        loc += 4;
		__stwbrx(dest, loc, src->rtype.pobj_info.alt_textures);         loc += 4;
	} else {
		__stwbrx(dest, loc, src->rtype.vclip_info.frametime);
		dest[loc+4] = src->rtype.vclip_info.framenum;                   loc += 72;
	}

	Assert(loc==SIZEOF_OBJECT);
}

void unpack_object(object *dest, ubyte *src) {
	u32 loc=0;

	dest->signature = __lwbrx(src, loc);                                loc += 4;
	dest->type = src[loc++];
	dest->id = src[loc++];
	dest->next = __lhbrx(src, loc);                                     loc += 2;
	dest->prev = __lhbrx(src, loc);                                     loc += 2;
	dest->control_type = src[loc++];
	dest->movement_type = src[loc++];
	dest->render_type = src[loc++];
	dest->flags = src[loc++];
	dest->segnum = __lhbrx(src, loc);                                   loc += 2;
	dest->attached_obj = __lhbrx(src, loc);                             loc += 2;

	dest->pos.x = __lwbrx(src, loc);                                    loc += 4;
	dest->pos.y = __lwbrx(src, loc);                                    loc += 4;
	dest->pos.z = __lwbrx(src, loc);                                    loc += 4;

	dest->orient.rvec.x = __lwbrx(src, loc);                            loc += 4;
	dest->orient.rvec.y = __lwbrx(src, loc);                            loc += 4;
	dest->orient.rvec.z = __lwbrx(src, loc);                            loc += 4;
	dest->orient.fvec.x = __lwbrx(src, loc);                            loc += 4;
	dest->orient.fvec.y = __lwbrx(src, loc);                            loc += 4;
	dest->orient.fvec.z = __lwbrx(src, loc);                            loc += 4;
	dest->orient.uvec.x = __lwbrx(src, loc);                            loc += 4;
	dest->orient.uvec.y = __lwbrx(src, loc);                            loc += 4;
	dest->orient.uvec.z = __lwbrx(src, loc);                            loc += 4;

	dest->size = __lwbrx(src, loc);                                     loc += 4;
	dest->shields = __lwbrx(src, loc);                                  loc += 4;

	dest->last_pos.x = __lwbrx(src, loc);                               loc += 4;
	dest->last_pos.y = __lwbrx(src, loc);                               loc += 4;
	dest->last_pos.z = __lwbrx(src, loc);                               loc += 4;

	dest->contains_type = src[loc++];
	dest->contains_id = src[loc++];
	dest->contains_count = src[loc++];
	dest->matcen_creator = src[loc++];
	dest->lifeleft = __lwbrx(src, loc);                                 loc += 4;

	dest->mtype.spin_rate.x = __lwbrx(src, loc);                        loc += 4;
	dest->mtype.spin_rate.y = __lwbrx(src, loc);                        loc += 4;
	dest->mtype.spin_rate.z = __lwbrx(src, loc);                        loc += 4;

	// optionally handle the rest of physics_info
	if (dest->movement_type==MT_PHYSICS) {
		dest->mtype.phys_info.thrust.x = __lwbrx(src, loc);             loc += 4;
		dest->mtype.phys_info.thrust.y = __lwbrx(src, loc);             loc += 4;
		dest->mtype.phys_info.thrust.z = __lwbrx(src, loc);             loc += 4;
		dest->mtype.phys_info.mass = __lwbrx(src, loc);                 loc += 4;
		dest->mtype.phys_info.drag = __lwbrx(src, loc);                 loc += 4;
		dest->mtype.phys_info.brakes = __lwbrx(src, loc);               loc += 4;
		dest->mtype.phys_info.rotvel.x = __lwbrx(src, loc);             loc += 4;
		dest->mtype.phys_info.rotvel.y = __lwbrx(src, loc);             loc += 4;
		dest->mtype.phys_info.rotvel.z = __lwbrx(src, loc);             loc += 4;
		dest->mtype.phys_info.rotthrust.x = __lwbrx(src, loc);          loc += 4;
		dest->mtype.phys_info.rotthrust.y = __lwbrx(src, loc);          loc += 4;
		dest->mtype.phys_info.rotthrust.z = __lwbrx(src, loc);          loc += 4;
		dest->mtype.phys_info.turnroll = __lhbrx(src, loc);             loc += 2;
		dest->mtype.phys_info.flags = __lhbrx(src, loc);                loc += 2;
	} else
		loc += 52;

	switch (dest->control_type) {

	case CT_WEAPON: // laser_info,laser_info
		dest->ctype.laser_info.parent_type = __lhbrx(src, loc);         loc += 2;
		dest->ctype.laser_info.parent_num = __lhbrx(src, loc);          loc += 2;
		dest->ctype.laser_info.parent_signature = __lwbrx(src, loc);    loc += 4;
		dest->ctype.laser_info.creation_time = __lwbrx(src, loc);       loc += 4;
		dest->ctype.laser_info.last_hitobj = __lhbrx(src, loc);         loc += 2;
		dest->ctype.laser_info.track_goal = __lhbrx(src, loc);          loc += 2;
		dest->ctype.laser_info.multiplier = __lwbrx(src, loc);          loc += 14;
		break;

	case CT_EXPLOSION: // explosion_info,expl_info
		dest->ctype.expl_info.spawn_time = __lwbrx(src, loc);           loc += 4;
		dest->ctype.expl_info.delete_time = __lwbrx(src, loc);          loc += 4;
		dest->ctype.expl_info.delete_objnum = __lhbrx(src, loc);        loc += 2;
		dest->ctype.expl_info.attach_parent = __lhbrx(src, loc);        loc += 2;
		dest->ctype.expl_info.prev_attach = __lhbrx(src, loc);          loc += 2;
		dest->ctype.expl_info.next_attach = __lwbrx(src, loc);          loc += 16;
		break;

	case CT_AI: // ai_static,ai_info
		dest->ctype.ai_info.behavior = src[loc++];
		memcpy(dest->ctype.ai_info.flags, src+loc, 11/*MAX_AI_FLAGS*/); loc += 11;
		dest->ctype.ai_info.hide_segment = __lhbrx(src, loc);           loc += 2;
		dest->ctype.ai_info.hide_index = __lhbrx(src, loc);             loc += 2;
		dest->ctype.ai_info.path_length = __lhbrx(src, loc);            loc += 2;
		dest->ctype.ai_info.cur_path_index = __lhbrx(src, loc);         loc += 2;
		dest->ctype.ai_info.follow_path_start_seg = __lhbrx(src, loc);  loc += 2;
		dest->ctype.ai_info.follow_path_end_seg = __lhbrx(src, loc);    loc += 2;
		dest->ctype.ai_info.danger_laser_signature = __lwbrx(src, loc); loc += 4;
		dest->ctype.ai_info.danger_laser_num = __lhbrx(src, loc);       loc += 2;
		break;

	case CT_LIGHT: // light_info,light_info
	case CT_POWERUP: // powerup_info,powerup_info
		dest->ctype.powerup_info.count = __lwbrx(src, loc);
		// vulcan cannon now comes with included ammo! But why is this special cased here?
		if (dest->control_type==CT_POWERUP && dest->id == POW_VULCAN_WEAPON)
			dest->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
	default:
		                                                                loc += 30;
	}

	// rtype always starts with an int
	dest->rtype.vclip_info.vclip_num = __lwbrx(src, loc);               loc += 4;
	if (dest->render_type==RT_MORPH || dest->render_type==RT_POLYOBJ) {
		int i;
		for (i=0; i < 10/*MAX_SUBMODELS*/; i++) {
			dest->rtype.pobj_info.anim_angles[i].p = __lhbrx(src, loc); loc += 2;
			dest->rtype.pobj_info.anim_angles[i].b = __lhbrx(src, loc); loc += 2;
			dest->rtype.pobj_info.anim_angles[i].h = __lhbrx(src, loc); loc += 2;
		}
		dest->rtype.pobj_info.subobj_flags = __lwbrx(src, loc);         loc += 4;
		dest->rtype.pobj_info.tmap_override = __lwbrx(src, loc);        loc += 4;
		dest->rtype.pobj_info.alt_textures = __lwbrx(src, loc);         loc += 4;
	} else {
		dest->rtype.vclip_info.frametime = __lwbrx(src, loc);
		dest->rtype.vclip_info.framenum = src[loc+4];                   loc += 72;
	}

	Assert(loc==SIZEOF_OBJECT);
}

#endif // NETWORK