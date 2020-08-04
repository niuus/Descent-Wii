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
#include <string.h>
#include <errno.h>
//#include <io.h>

#include "error.h"

#include "gameseq.h"
#include "player.h"
#include "playsave.h"
#include "input.h"
#include "kconfig.h"
#include "digi.h"
#include "newmenu.h"
#include "joydefs.h"
#include "palette.h"
#include "multi.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "mono.h"
#include "state.h"
#include "byteswap.h"

#define SAVE_FILE_ID		0x44504C52 /*'DPLR'*/

//this is for version 6 and above
typedef struct save_info {
	int	id;
	short	saved_game_version,player_struct_version;
	int	n_highest_levels;				//how many highest levels are saved
	int	default_difficulty_level;
	int	default_leveling_on;
} save_info;

typedef struct hli {
	char	shortname[9];
	ubyte	level_num;
} hli;

int n_highest_levels;

hli highest_levels[MAX_MISSIONS];

#define SAVED_GAME_VERSION		7		//increment this every time saved_game struct changes

//version 5 -> 6: added new highest level information
//version 6 -> 7: stripped out the old saved_game array.

//the shareware is level 4

#define COMPATIBLE_SAVED_GAME_VERSION		4
#define COMPATIBLE_PLAYER_STRUCT_VERSION	16

typedef struct saved_game {
	char		name[GAME_NAME_LEN+1];		//extra char for terminating zero
	player	player;
	int		difficulty_level;		//which level game is played at
	int		primary_weapon;		//which weapon selected
	int		secondary_weapon;		//which weapon selected
	int		cockpit_mode;			//which cockpit mode selected
	int		window_w,window_h;	//size of player's window
	int		next_level_num;		//which level we're going to
	int		auto_leveling_on;		//does player have autoleveling on?
} saved_game;

saved_game saved_games[N_SAVE_SLOTS];

int Default_leveling_on=1;

void init_game_list()
{
	int i;

	for (i=0;i<N_SAVE_SLOTS;i++)
		saved_games[i].name[0] = 0;
}

int new_player_config()
{
	int i,j,control_choice;
	newmenu_item m[7];

//RetrySelection:
	for (i=0; i<CONTROL_MAX_TYPES; i++ )	{
		m[i].type = NM_TYPE_MENU; m[i].text = CONTROL_TEXT(i);
	}

	control_choice = Config_control_type;				// Assume keyboard

	control_choice = newmenu_do1( NULL, TXT_CHOOSE_INPUT, CONTROL_MAX_TYPES, m, NULL, control_choice );

	switch (control_choice) {
		case 1:
			Config_control_type = CONTROL_NUNCHUK;
			break;
		case 2:
			Config_control_type = CONTROL_CLASSIC;
			break;
		default:
			Config_control_type = CONTROL_WIIMOTE;
	}

	for (i=0;i<CONTROL_MAX_TYPES; i++ )
		for (j=0;j<MAX_CONTROLS; j++ )
			kconfig_settings[i][j] = default_kconfig_settings[i][j];
	kc_set_controls();

//	Config_control_type = control_choice;

//	if ( Config_control_type==CONTROL_THRUSTMASTER_FCS)	{
//		i = nm_messagebox( TXT_IMPORTANT_NOTE, 2, "Choose another", TXT_OK, TXT_FCS );
//		if (i==0) goto RetrySelection;
//	}

	if ( (Config_control_type>0) && 	(Config_control_type<CONTROL_MAX_TYPES) )	{
		joydefs_set_type(Config_control_type);
		joydefs_calibrate();
	}

	Player_default_difficulty = 1;
	Auto_leveling_on = Default_leveling_on = 1;
	n_highest_levels = 1;
	highest_levels[0].shortname[0] = 0;			//no name for mission 0
	highest_levels[0].level_num = 1;				//was highest level in old struct
	Config_joystick_sensitivity = 8;

	// Default taunt macros
	#ifdef NETWORK
	strcpy(Network_message_macro[0], TXT_DEF_MACRO_1);
	strcpy(Network_message_macro[1], TXT_DEF_MACRO_2);
	strcpy(Network_message_macro[2], TXT_DEF_MACRO_3);
	strcpy(Network_message_macro[3], TXT_DEF_MACRO_4);
	#endif

	return 1;
}

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	char filename[64];
	FILE *file;
	save_info info;
	int errno_ret = 0;

	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	//sprintf(filename,"/Descent/%s.plr",Players[Player_num].callsign);
	sprintf(filename, "%s.plr", Players[Player_num].callsign);
	file = fopen(filename,"rb");

#if 0
	//check filename
	if (file && isatty(fileno(file))) {
		//if the callsign is the name of a tty device, prepend a char
		fclose(file);
		sprintf(filename,"$%.7s.plr",Players[Player_num].callsign);
		file = fopen(filename,"rb");
	}
#endif

	if (!file) {
		return errno;
	}

	if (fread(&info,sizeof(info),1,file) != 1) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}
	info.id = swapint(info.id);
	info.saved_game_version = swapshort(info.saved_game_version);
	info.player_struct_version = swapshort(info.player_struct_version);
	info.n_highest_levels = swapint(info.n_highest_levels);
	info.default_difficulty_level = swapint(info.default_difficulty_level);
	info.default_leveling_on = swapint(info.default_leveling_on);

	if (info.id!=SAVE_FILE_ID) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid player file");
		fclose(file);
		return -1;
	}

	if (info.saved_game_version<COMPATIBLE_SAVED_GAME_VERSION || info.player_struct_version<COMPATIBLE_PLAYER_STRUCT_VERSION) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		fclose(file);
		return -1;
	}

	if (info.saved_game_version <= 5) {

		//deal with old-style highest level info

		n_highest_levels = 1;

		highest_levels[0].shortname[0] = 0;							//no name for mission 0
		highest_levels[0].level_num = info.n_highest_levels;	//was highest level in old struct

		//This hack allows the player to start on level 8 if he's made it to
		//level 7 on the shareware.  We do this because the shareware didn't
		//save the information that the player finished level 7, so the most
		//we know is that he made it to level 7.
		if (info.n_highest_levels==7)
			highest_levels[0].level_num = 8;

	}
	else {	//read new highest level info

		n_highest_levels = info.n_highest_levels;

		if (fread(highest_levels,sizeof(hli),n_highest_levels,file) != n_highest_levels) {
			errno_ret = errno;
			fclose(file);
			return errno_ret;
		}
	}

	Player_default_difficulty = info.default_difficulty_level;
	Default_leveling_on = info.default_leveling_on;

	if ( info.saved_game_version < 7 )	{			// Read old saved games.
#if 0			// NEED TO FIX THIS!!!!!!!!!!!!!!!!!!!!
typedef struct saved_game {
	char		name[GAME_NAME_LEN+1];		//extra char for terminating zero
	player	player;
} saved_game;
		if (fread(saved_games,sizeof(saved_games),1,file) != 1) {
			errno_ret = errno;
			fclose(file);
			return errno_ret;
		}
		for (i = 0; i < N_SAVE_SLOTS; i++) {
			saved_games[i].difficulty = swapint(saved_games[i].difficulty);
			saved_games[i].primary_weapon = swapint(saved_games[i].primary_weapon);
			saved_games[i].secondary_weapon = swapint(saved_games[i].secondary_weapon);
			saved_games[i].cockpit_mode = swapint(saved_games[i].cockpit_mode);
			saved_games[i].window_w = swapint(saved_games[i].window_w);
			saved_games[i].window_h = swapint(saved_games[i].window_h);
			saved_games[i].window_h = swapint(saved_games[i].window_h);
			saved_games[i].next_level_num = swapint(saved_games[i].next_level_num);
			saved_games[i].auto_leveling_on = swapint(saved_games[i].auto_leveling_on);
		}
#endif
	}

	//read taunt macros
	{
		int i,len;

#ifdef MAC_SHAREWARE
		len = 35;
#else
		len = (info.saved_game_version == 4)?SHAREWARE_MAX_MESSAGE_LEN:MAX_MESSAGE_LEN;
#endif

		#ifdef NETWORK
		for (i = 0; i < 4; i++)
			if (fread(Network_message_macro[i], len, 1, file) != 1)
				{errno_ret = errno; break;}
		#else
		i = 0;
		fseek( file, MAX_MESSAGE_LEN*4, SEEK_CUR );
		#endif
	}

	//read kconfig data
	{
		if (fread( kconfig_settings, MAX_CONTROLS*CONTROL_MAX_TYPES, 1, file )!=1)
			errno_ret=errno;
		else if (fread(&Config_control_type, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
		else if (fread(&Config_joystick_sensitivity, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;

		if (Config_control_type < 0 || Config_control_type >= CONTROL_MAX_TYPES)
			Config_control_type = CONTROL_WIIMOTE;

		if (errno_ret==0)	{
//			if ( (Config_control_type>0) && 	(Config_control_type<5) )	{
//				joydefs_calibrate();
//			}
			joydefs_set_type(Config_control_type);
			kc_set_controls();
		}
	}

	if (fclose(file) && errno_ret==0)
		errno_ret = errno;

	if ( info.saved_game_version == COMPATIBLE_SAVED_GAME_VERSION )		{
		int i;

		Assert( N_SAVE_SLOTS == 10 );

		for (i=0; i<N_SAVE_SLOTS; i++ )	{
			if ( saved_games[i].name[0] )	{
				state_save_old_game(i, saved_games[i].name, &saved_games[i].player,
             		saved_games[i].difficulty_level, saved_games[i].primary_weapon,
          			saved_games[i].secondary_weapon, saved_games[i].next_level_num );
			}
		}
		write_player_file();
	}

	return errno_ret;

}

//finds entry for this level in table.  if not found, returns ptr to
//empty entry.  If no empty entries, takes over last one
int find_hli_entry()
{
	int i;

	for (i=0;i<n_highest_levels;i++) {
#ifdef MAC_SHAREWARE
		if (!stricmp(highest_levels[i].shortname,""))
			break;
#else
		if (!stricmp(highest_levels[i].shortname,Mission_list[Current_mission_num].filename))
			break;
#endif
	}

#ifdef MAC_SHAREWARE
	Assert( i < n_highest_levels );			// should *always* have built in mission highest level num
#else
	if (i==n_highest_levels) {		//not found.  create entry

		if (i==MAX_MISSIONS)
			i--;		//take last entry
		else
			n_highest_levels++;

		strcpy(highest_levels[i].shortname,Mission_list[Current_mission_num].filename);
		highest_levels[i].level_num = 0;
	}
#endif

	return i;
}

//set a new highest level for player for this mission
void set_highest_level(int levelnum)
{
	int ret,i;

	if ((ret=read_player_file()) != 0)
//		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return;

	i = find_hli_entry();

	if (levelnum > highest_levels[i].level_num)
		highest_levels[i].level_num = levelnum;

	write_player_file();
}

//gets the player's highest level from the file for this mission
int get_highest_level(void)
{
	int i;
	int highest_saturn_level = 0;
	read_player_file();

#ifndef MAC_SHAREWARE
#ifndef SATURN
	if (strlen(Mission_list[Current_mission_num].filename)==0 )	{
		for (i=0;i<n_highest_levels;i++)
			if (!stricmp(highest_levels[i].shortname, "DESTSAT")) 	//	Destination Saturn.
		 		highest_saturn_level = highest_levels[i].level_num;
	}
#endif
#endif

	i = highest_levels[find_hli_entry()].level_num;
	if ( highest_saturn_level > i )
   	i = highest_saturn_level;
	return i;
}


//write out player's saved games.  returns errno (0 == no error)
int write_player_file()
{
	char filename[64];
	FILE *file;
	save_info info;
	int errno_ret;

	#ifdef APPLE_OEM		// no saving of player files in Apple OEM version
	return 0;
	#endif

	errno_ret = WriteConfigFile();

	info.id = swapint(SAVE_FILE_ID);
	info.saved_game_version = swapshort(SAVED_GAME_VERSION);
	info.player_struct_version = swapshort(PLAYER_STRUCT_VERSION);
	info.default_difficulty_level = swapint(Player_default_difficulty);
	info.default_leveling_on = swapint(Auto_leveling_on);

	info.n_highest_levels = swapint(n_highest_levels);

	//sprintf(filename,"/Descent/%s.plr",Players[Player_num].callsign);
	sprintf(filename, "%s.plr", Players[Player_num].callsign);
	file = fopen(filename,"wb");

#if 0
	//check filename
	if (file && isatty(fileno(file))) {

		//if the callsign is the name of a tty device, prepend a char

		fclose(file);
		sprintf(filename,"$%.7s.plr",Players[Player_num].callsign);
		file = fopen(filename,"wb");
	}
#endif

	if (!file)
		return errno;

	errno_ret = 0;

	if (fwrite(&info,sizeof(info),1,file) != 1) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}

	//write higest level info
	if ((fwrite(highest_levels, sizeof(hli), n_highest_levels, file) != n_highest_levels)) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}

//	if (fwrite(saved_games,sizeof(saved_games),1,file) != 1) {
//		errno_ret = errno;
//		fclose(file);
//		return errno_ret;
//	}

	#ifdef NETWORK
	if ((fwrite(Network_message_macro, MAX_MESSAGE_LEN, 4, file) != 4)) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}
	#else
	fseek( file, MAX_MESSAGE_LEN * 4, SEEK_CUR );
	#endif

	//write kconfig info
	{
		if (fwrite( kconfig_settings, MAX_CONTROLS*CONTROL_MAX_TYPES, 1, file )!=1)
			errno_ret=errno;
		else if (fwrite( &Config_control_type, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
		else if (fwrite( &Config_joystick_sensitivity, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
	}

	if (fclose(file))
		errno_ret = errno;

	if (errno_ret != 0) {
		remove(filename);			//delete bogus file
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s",TXT_ERROR_WRITING_PLR, strerror(errno_ret));
	}


	return errno_ret;

}

//returns errno (0 == no error)
int save_player_game(int slot_num,char *text)
{
	int ret;

	if ((ret=read_player_file()) != 0)
//		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return ret;

	Assert(slot_num < N_SAVE_SLOTS);

	strcpy(saved_games[slot_num].name,text);

	saved_games[slot_num].player = Players[Player_num];

	saved_games[slot_num].difficulty_level	= Difficulty_level;
	saved_games[slot_num].auto_leveling_on	= Auto_leveling_on;
	saved_games[slot_num].primary_weapon	= Primary_weapon;
	saved_games[slot_num].secondary_weapon	= Secondary_weapon;
	saved_games[slot_num].cockpit_mode		= Cockpit_mode;
	saved_games[slot_num].window_w			= Game_window_w;
	saved_games[slot_num].window_h			= Game_window_h;
	saved_games[slot_num].next_level_num	= Next_level_num;

	return write_player_file();
}


//returns errno (0 == no error)
int load_player_game(int slot_num)
{
	char save_callsign[CALLSIGN_LEN+1];
	int ret;

	Assert(slot_num < N_SAVE_SLOTS);

	if ((ret=read_player_file()) != 0)
		return ret;

	Assert(saved_games[slot_num].name[0] != 0);

	strcpy(save_callsign,Players[Player_num].callsign);
	Players[Player_num] = saved_games[slot_num].player;
	strcpy(Players[Player_num].callsign,save_callsign);

	Difficulty_level	= saved_games[slot_num].difficulty_level;
	Auto_leveling_on	= saved_games[slot_num].auto_leveling_on;
	Primary_weapon		= saved_games[slot_num].primary_weapon;
	Secondary_weapon	= saved_games[slot_num].secondary_weapon;
	Cockpit_mode		= saved_games[slot_num].cockpit_mode;
	Game_window_w		= saved_games[slot_num].window_w;
	Game_window_h		= saved_games[slot_num].window_h;

	Players[Player_num].level = saved_games[slot_num].next_level_num;

	return 0;
}

//fills in a list of pointers to strings describing saved games
//returns the number of non-empty slots
//returns -1 if this is a new player
int get_game_list(char *game_text[N_SAVE_SLOTS])
{
	int i,count,ret;

	ret = read_player_file();

	for (i=count=0;i<N_SAVE_SLOTS;i++) {
		if (game_text)
			game_text[i] = saved_games[i].name;

		if (saved_games[i].name[0])
			count++;
	}

	return (ret==0)?count:-1;		//-1 means new file was created

}

//update the player's highest level.  returns errno (0 == no error)
int update_player_file()
{
	int ret;

	if ((ret=read_player_file()) != 0)
//		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return ret;

	return write_player_file();
}
