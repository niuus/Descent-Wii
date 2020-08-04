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

//static char copyright[] = "DESCENT   COPYRIGHT (C) 1994,1995 PARALLAX SOFTWARE CORPORATION";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef PROFILE
#include <profiler.h>
#endif

#include "gr.h"
#include "mono.h"
#include "input.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "mem.h"
#include "textures.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "wall.h"
#include "switch.h"
#include "polyobj.h"
#include "effects.h"
#include "digi.h"
#include "iff.h"
#include "pcx.h"
#include "palette.h"
#include "args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "text.h"
#include "ipx.h"
#include "newdemo.h"
#include "network.h"
#include "modem.h"
#include "gamefont.h"
#include "kconfig.h"
#include "arcade.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "joydefs.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "cdrom.h"
#include "gameseq.h"
#include "strutil.h"
#include "gtimer.h"

#include "wiisys.h"
//#include "appltalk.h"

#ifdef EDITOR
#include "editor\editor.h"
#include "editor\kdefs.h"
#include "ui.h"
#endif

#include "vers_id.h"

static const char desc_id_checksum_str[] = DESC_ID_CHKSUM;
char desc_id_exit_num = 0;

int Function_mode=FMODE_MENU;		//game or editor?
int Screen_mode=-1;					//game screen or editor screen?

//--unused-- grs_bitmap Inferno_bitmap_title;

int WVIDEO_running=0;		//debugger can set to 1 if running

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

//--unused-- int Cyberman_installed=0;			// SWIFT device present

//void install_int3_handler(void);

int init_globals(void);

//int __far descent_critical_error_handler( unsigned deverr, unsigned errcode, unsigned far * devhdr );

#ifndef NDEBUG
void do_heap_check()
{
}
#endif

int registered_copy=0;
char name_copy[sizeof(DESC_ID_STR)];

void
check_id_checksum_and_date()
{
#if 0
	char name[128];
	Handle name_handle, time_handle, checksum_handle;
	int i, found;
	unsigned long *checksum, test_checksum;
	time_t current_time, saved_time;

	name_handle = GetResource('krAm', 1001);
	time_handle = GetResource('krAm', 1002);
	checksum_handle = GetResource('krAm', 1003);
	if ((name_handle == NULL) || (time_handle == NULL) || (checksum_handle == NULL)) {
		desc_id_exit_num = 2;
		return;
	}
	if (!strcmp((char *)(*name_handle), "Parallax")) {
		if ((*((unsigned long *)(*time_handle)) != 0xaabbccdd) || (*((unsigned long *)(*checksum_handle)) != 0xffee1122)) {
			desc_id_exit_num = 2;
			return;
		}
		return;
	}
	saved_time = (time_t)*((unsigned long *)(*time_handle));

	strcpy(name, (char *)(*name_handle));
	strcpy(name_copy,name);
	registered_copy = 1;

	current_time = time(NULL);
	if (current_time >= saved_time)
		desc_id_exit_num = 1;

	test_checksum = 0;
	for (i = 0; i < strlen(name); i++) {
		found = 0;
		test_checksum += name[i];
		if (((test_checksum / 2) * 2) != test_checksum)
			found = 1;
		test_checksum = test_checksum >> 1;
		if (found)
			test_checksum |= 0x80000000;
	}
	if (test_checksum != *((unsigned long *)(*checksum_handle)))
		desc_id_exit_num = 2;
#endif
}

int init_graphics()
{
	return 0;
}

extern fix fixed_frametime;

void check_dos_version()
{
}

void change_to_dir(char *cmd_line)
{
}

void dos_check_file_handles(int num_required)
{
}

#define NEEDED_DOS_MEMORY   			( 300*1024)		// 300 K
#define NEEDED_LINEAR_MEMORY 			(7680*1024)		// 7.5 MB
#define LOW_PHYSICAL_MEMORY_CUTOFF	(5*1024*1024)	// 5.0 MB
#define NEEDED_PHYSICAL_MEMORY		(2000*1024)		// 2000 KB

extern int piggy_low_memory;

void mem_int_to_string( int number, char *dest )
{
	int i,l,c;
	char buffer[20],*p;

	sprintf( buffer, "%d", number );

	l = strlen(buffer);
	if (l<=3) {
		// Don't bother with less than 3 digits
		sprintf( dest, "%d", number );
		return;
	}

	c = 0;
	p=dest;
	for (i=l-1; i>=0; i-- )	{
		if (c==3) {
			*p++=',';
			c = 0;
		}
		c++;
		*p++ = buffer[i];
	}
	*p++ = '\0';
	strrev(dest);
}

void check_memory()
{
}

int Inferno_verbose = 0;

//NO_STACK_SIZE_CHECK uint * stack, *stack_ptr;
//NO_STACK_SIZE_CHECK int stack_size, unused_stack_space;
//NO_STACK_SIZE_CHECK int sil;
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK int main(int argc,char **argv)
//NO_STACK_SIZE_CHECK {
//NO_STACK_SIZE_CHECK 	uint ret_value;
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK 	unused_stack_space = 0;
//NO_STACK_SIZE_CHECK 	stack = &ret_value;
//NO_STACK_SIZE_CHECK 	stack_size = stackavail()/4;
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK 	for ( sil=0; sil<stack_size; sil++ )	{
//NO_STACK_SIZE_CHECK 		stack--;
//NO_STACK_SIZE_CHECK 		*stack = 0xface0123;
//NO_STACK_SIZE_CHECK 	}
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK 	ret_value = descent_main( argc, argv );		// Rename main to be descent_main
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK 	for ( sil=0; sil<stack_size; sil++ )	{
//NO_STACK_SIZE_CHECK 		if ( *stack == 0xface0123 )
//NO_STACK_SIZE_CHECK 			unused_stack_space++;
//NO_STACK_SIZE_CHECK 		stack++;
//NO_STACK_SIZE_CHECK 	}
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK 	mprintf(( 0, "Program used %d/%d stack space\n", (stack_size - unused_stack_space)*4, stack_size*4 ));
//NO_STACK_SIZE_CHECK 	key_getch();
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK 	return ret_value;
//NO_STACK_SIZE_CHECK }

extern int digi_timer_rate;

int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

#if 0
#pragma off (check_stack)
int __far descent_critical_error_handler(unsigned deverror, unsigned errcode, unsigned __far * devhdr )
{
	devhdr = devhdr;
	descent_critical_error++;
	descent_critical_deverror = deverror;
	descent_critical_errcode = errcode;
	return _HARDERR_FAIL;
}
void chandler_end (void)  // dummy functions
{
}
#pragma on (check_stack)
#endif // #if 0

extern int Network_allow_socket_changes;

extern void vfx_set_palette_sub(ubyte *);

extern int Game_vfx_flag;
extern int Game_victor_flag;
extern int Game_vio_flag;
extern int Game_3dmax_flag;
extern int VR_low_res;

#ifdef SATURN
char destsat_cdpath[128] = "";
int find_descent_cd();
#endif

extern int Config_vr_type;
extern int Config_vr_tracking;

void show_order_form()
{
	int pcx_error;
	ubyte title_pal[768];
	char	exit_screen[16];

	gr_set_current_canvas( NULL );
	gr_palette_clear();

	key_flush();

#ifdef MAC_SHAREWARE
	#ifdef APPLE_OEM
	strcpy(exit_screen, "apple.pcx");
	#else
	strcpy(exit_screen, "order01.pcx");
	#endif
#else
	#ifdef SATURN
		strcpy(exit_screen, "order01.pcx");
	#else
		strcpy(exit_screen, "warning.pcx");
	#endif
#endif
	if ((pcx_error=pcx_read_bitmap( exit_screen, &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, title_pal ))==PCX_ERROR_NONE) {
		int k;

//		vfx_set_palette_sub( title_pal );
		bitblt_to_screen();
		gr_palette_fade_in( title_pal, 32, 0 );
		key_flush();
		while (1) {
			k = key_inkey();
			if ( k != 0 || mouse_went_down(0) )
				break;
		}
		gr_palette_fade_out( title_pal, 32, 0 );
	}
	key_flush();

#ifdef MAC_SHAREWARE
	{
		struct tm expire_time;
		time_t cur_time, dead_time;

		cur_time = time(NULL);
		expire_time.tm_mday = 10;
		expire_time.tm_mon = 11;
		expire_time.tm_year = 95;
		dead_time = mktime(&expire_time);
		if (cur_time > dead_time)
			return;

		strcpy(exit_screen, "contest.pcx");
		if ((pcx_error=pcx_read_bitmap( exit_screen, &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, title_pal ))==PCX_ERROR_NONE) {
			int k;

	//		vfx_set_palette_sub( title_pal );
			bitblt_to_screen();
			gr_palette_fade_in( title_pal, 32, 0 );
			key_flush();
			while (1) {
				k = key_inkey();
				if ( k != 0 || mouse_went_down(0) )
					break;
			}
			gr_palette_fade_out( title_pal, 32, 0 );
		}
		key_flush();
	}
#endif
}


int main(int argc,char **argv)
{
	int t;
	ubyte title_pal[768];

//	error_init(NULL);

#ifdef PROFILE
	if (ProfilerInit(collectSummary, PPCTimeBase, 200, 50))
		Error("Profile Init failed");
	ProfilerSetStatus(0);
#endif

	if (wii_init()) {
		Error("Wii initialization failed!");
		return 1;
	}

	setbuf(stdout, NULL);	// unbuffered output via printf

	InitArgs( argc,argv );

	if ( FindArg( "-verbose" ) )
		Inferno_verbose = 1;

	//change_to_dir(argv[0]);

#ifdef SATURN
	int i=find_descent_cd();
	if ( i>0 )		{
		sprintf( destsat_cdpath, "%c:\\descent\\", i +'a' - 1  );
		cfile_use_alternate_hogdir( destsat_cdpath );
	} else {
		printf( "\n\n" );
		printf("Couldn't find the 'Descent: Destination Saturn' CD-ROM.\n" );
		printf("Please make sure that it is in your CD-ROM drive and\n" );
		printf("that your CD-ROM drivers are loaded correctly.\n" );
		exit(1);
	}
#endif

	load_text();

//	set_exit_message("\n\n%s", TXT_THANKS);

//	printf("\nDESCENT   %s\n", VERSION_NAME);
//	printf("%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);

	check_id_checksum_and_date();

	if (FindArg( "-?" ) || FindArg( "-help" ) || FindArg( "?" ) )	{

		printf( "%s\n", TXT_COMMAND_LINE_0 );
		printf("  -Iglasses      %s\n", TXT_IGLASSES );
		printf("  -VioTrack <n>  %s n\n",TXT_VIOTRACK );
		printf("  -3dmaxLo       %s\n",TXT_KASAN );
		printf("                 %s\n",TXT_KASAN_2 );
		printf("  -3dmaxHi       %s\n",TXT_3DMAX );
		printf( "%s\n", TXT_COMMAND_LINE_1 );
		printf( "%s\n", TXT_COMMAND_LINE_2 );
		printf( "%s\n", TXT_COMMAND_LINE_3 );
		printf( "%s\n", TXT_COMMAND_LINE_4 );
		printf( "%s\n", TXT_COMMAND_LINE_5 );
//		printf( "\n");
		printf( "%s\n", TXT_COMMAND_LINE_6 );
		printf( "%s\n", TXT_COMMAND_LINE_7 );
		printf( "%s\n", TXT_COMMAND_LINE_8 );
//		printf( "\n");
		printf( "\n%s\n",TXT_PRESS_ANY_KEY3);
//		getch();
		printf( "\n" );
		printf( "%s\n", TXT_COMMAND_LINE_9);
		printf( "%s\n", TXT_COMMAND_LINE_10);
		printf( "%s\n", TXT_COMMAND_LINE_11);
		printf( "%s\n", TXT_COMMAND_LINE_12);
		printf( "%s\n", TXT_COMMAND_LINE_13);
		printf( "%s\n", TXT_COMMAND_LINE_14);
		printf( "%s\n", TXT_COMMAND_LINE_15);
		printf( "%s\n", TXT_COMMAND_LINE_16);
		printf( "%s\n", TXT_COMMAND_LINE_17);
		printf( "%s\n", TXT_COMMAND_LINE_18);
      printf( "  -DynamicSockets %s\n", TXT_SOCKET);
      printf( "  -NoFileCheck    %s\n", TXT_NOFILECHECK);
//		set_exit_message("");
		return(0);
	}

//	printf("\n%s\n", TXT_HELP);

	#ifdef PASSWORD
	if ((t = FindArg("-pswd")) != 0) {
		int	n;
		byte	*pp = Side_to_verts;
		int ch;
		for (n=0; n<6; n++)
			for (ch=0; ch<strlen(Args[t+1]); ch++)
				*pp++ ^= Args[t+1][ch];
	}
	else
		Error("Invalid processor");		//missing password
	#endif

	if ( FindArg( "-autodemo" ))
		Auto_demo = 1;

	#ifndef RELEASE
	if ( FindArg( "-noscreens" ) )
		Skip_briefing_screens = 1;
	#endif

	Lighting_on = 1;

	if ( !FindArg( "-nodoscheck" ))
		check_dos_version();

	if ( !FindArg( "-nofilecheck" ))
		dos_check_file_handles(5);

	if ( !FindArg( "-nomemcheck" ))
		check_memory();

	strcpy(Menu_pcx_name, "menu.pcx");	//	Used to be menu2.pcx.

	if (init_graphics()) return 1;

	#ifndef NDEBUG
//		minit();
//		mopen( 0, 9, 1, 78, 15, "Debug Spew");
//		mopen( 1, 2, 1, 78,  5, "Errors & Serious Warnings");
	#endif

	if (!WVIDEO_running)
		mprintf((0,"WVIDEO_running = %d\n",WVIDEO_running));

	//if (!WVIDEO_running) install_int3_handler();

	//lib_init("INFERNO.DAT");

	timer_init();
	key_init();
//	div0_init(DM_ERROR);
	//------------ Init sound ---------------
	if (!FindArg( "-nosound" ))	{
		if (digi_init())	{
//			printf( "\n%s\n", TXT_PRESS_ANY_KEY3);
//			key_getch();
			mprintf ((0, "Error initializing digi drivers.\n"));
		}
	} else {
		if (Inferno_verbose) printf( "\n%s",TXT_SOUND_DISABLED );
	}
	ReadConfigFile();

#ifdef NETWORK
	if (!FindArg( "-nonetwork" ))	{
		int socket=0, showaddress=0;
		int ipx_error;
		if (Inferno_verbose) printf( "\n%s ", TXT_INITIALIZING_NETWORK);
		if ((t=FindArg("-socket")))
			socket = atoi( Args[t+1] );
		if ( FindArg("-showaddress") ) showaddress=1;
		if ((ipx_error=ipx_init(IPX_DEFAULT_SOCKET+socket,showaddress))==0)	{
  			if (Inferno_verbose) printf( "%s %d.\n", TXT_IPX_CHANNEL, socket );
		} else {
			switch( ipx_error )	{
			case 3: 	if (Inferno_verbose) printf( "%s\n", TXT_NO_NETWORK); break;
			case -2: if (Inferno_verbose) printf( "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET+socket); break;
			case -4: if (Inferno_verbose) printf( "%s\n", TXT_MEMORY_IPX ); break;
			default:
				if (Inferno_verbose) printf( "%s %d", TXT_ERROR_IPX, ipx_error );
			}
			if (Inferno_verbose) printf( "%s\n",TXT_NETWORK_DISABLED);
			Network_active = 0;		// Assume no network
		}
		ipx_read_user_file( "descent.usr" );
		ipx_read_network_file( "descent.net" );
		if ( FindArg( "-dynamicsockets" ))
			Network_allow_socket_changes = 1;
		else
			Network_allow_socket_changes = 0;

	} else {
		if (Inferno_verbose) printf( "%s\n", TXT_NETWORK_DISABLED);
		Network_active = 0;		// Assume no network
	}

#ifdef SERIAL
	if (!FindArg("-noserial"))
	{
		serial_active = 1;
	}
	else
	{
		serial_active = 0;
	}
#endif
#endif

	if (Inferno_verbose) printf( "\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if ((t=gr_init( SM_ORIGINAL ))!=0)
		Error(TXT_CANT_INIT_GFX,t);
	// Load the palette stuff. Returns non-zero if error.
	mprintf( (0, "Going into graphics mode..." ));
	gr_set_mode(SM_320x200x8);
	if (!FindArg( "-nomouse" ))	{
		if (Inferno_verbose) printf( "\n%s", TXT_VERBOSE_4);
		if (FindArg( "-nocyberman" ))
			mouse_init(0);
		else
			mouse_init(1);
	} else {
	 	if (Inferno_verbose) printf( "\n%s", TXT_VERBOSE_5);
	}
	hide_cursor();
	if (!FindArg( "-nojoystick" ))	{
		if (Inferno_verbose) printf( "\n%s", TXT_VERBOSE_6);
		joy_init();
	} else {
		if (Inferno_verbose) printf( "\n%s", TXT_VERBOSE_10);
	}

// render buffers must be set after gr_init since we need to allocate
// GWorld stuff in case of compatibility mode.

	game_init_render_buffers(SM_640x480V, 640, 480, 0);


	mprintf( (0, "\nInitializing palette system..." ));
	gr_use_palette_table( "PALETTE.256" );
	mprintf( (0, "\nInitializing font system..." ));
	gamefont_init();	// must load after palette data loaded.
	#ifndef APPLE_OEM
	songs_play_song( SONG_TITLE, 1 );
	#endif

	show_title_screen( "iplogo1.pcx", 1 );
	show_title_screen( "logo.pcx", 1 );

	{
		//grs_bitmap title_bm;
		int pcx_error;
		char filename[14];

		strcpy(filename, "descent.pcx");

		if ((pcx_error=pcx_read_bitmap( filename, &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, title_pal ))==PCX_ERROR_NONE)	{
//			vfx_set_palette_sub( title_pal );
			gr_palette_clear();
//			gr_bitmap( 0, 0, &title_bm );
//			bitblt_to_screen();
			gr_palette_fade_in( title_pal, 32, 0 );
			//free(title_bm.bm_data);
		} else {
			gr_close();
			Error( "Couldn't load pcx file '%s', PCX load error: %s\n",filename, pcx_errormsg(pcx_error));
		}
	}

	if (init_globals())
		Error("Error initing global vars.");

#ifdef EDITOR
	if ( !FindArg("-nobm") )
		bm_init_use_tbl();
	else
		bm_init();
#else
		bm_init();
#endif

	if ( FindArg( "-norun" ) )
		return(0);

	mprintf( (0, "\nInitializing 3d system..." ));
	g3_init();
	mprintf( (0, "\nInitializing texture caching system..." ));
	texmerge_init( 10 );		// 10 cache bitmaps
	mprintf( (0, "\nRunning game...\n" ));
	set_screen_mode(SCREEN_MENU);

	init_game();
	set_detail_level_parameters(Detail_level);

	Players[Player_num].callsign[0] = '\0';
	if (!Auto_demo) 	{
		key_flush();
		RegisterPlayer();		//get player's name
	}

	gr_palette_fade_out( title_pal, 32, 0 );

	//check for special stamped version
	if (registered_copy) {
		nm_messagebox("EVALUATION COPY",1,"Continue",
			"This special evaluation copy\n"
			"of DESCENT has been issued to:\n\n"
			"%s\n"
			"\n\n    NOT FOR DISTRIBUTION",
			name_copy);

		gr_palette_fade_out( gr_palette, 32, 0 );
	}

	//kconfig_load_all();

	Game_mode = GM_GAME_OVER;

	if (Auto_demo)	{
		newdemo_start_playback("DESCENT.DEM");
		if (Newdemo_state == ND_STATE_PLAYBACK )
			Function_mode = FMODE_GAME;
	}

#ifndef MAC_SHAREWARE
	build_mission_list(0);		// This also loads mission 0.
#endif

	#ifdef APPLE_OEM
	StartNewGame(1);
	#endif

	while (Function_mode != FMODE_EXIT)
	{
		switch( Function_mode )	{
		case FMODE_MENU:
			if ( Auto_demo )	{
				newdemo_start_playback(NULL);		// Randomly pick a file
				if (Newdemo_state != ND_STATE_PLAYBACK)
					Error("No demo files were found for autodemo mode!");
			} else {
				DoMenu();
#ifdef EDITOR
				if ( Function_mode == FMODE_EDITOR )	{
					create_new_mine();
					SetPlayerFromCurseg();
				}
#endif
			}
			break;
		case FMODE_GAME:
			#ifdef EDITOR
				keyd_editor_mode = 0;
			#endif
			game();
			if ( Function_mode == FMODE_MENU )
				songs_play_song( SONG_TITLE, 1 );
			break;
		#ifdef EDITOR
		case FMODE_EDITOR:
			keyd_editor_mode = 1;
			editor();
			_harderr( descent_critical_error_handler );		// Reinstall game error handler
			if ( Function_mode == FMODE_GAME ) {
				Game_mode = GM_EDITOR;
				editor_reset_stuff_on_level();
				N_players = 1;
			}
			break;
		#endif
		default:
			Error("Invalid function mode %d",Function_mode);
		}
	}

	WriteConfigFile();

	#ifndef RELEASE
	if (!FindArg( "-notitles" ))
	#endif
		//NOTE LINK TO ABOVE!!
	#ifndef EDITOR
		show_order_form();
	#endif

#ifdef PROFILE
	ProfilerDump("\pdescent.prof");
	ProfilerTerm();
#endif

//	#ifndef NDEBUG
//	if ( FindArg( "-showmeminfo" ) )
//		show_mem_info = 1;		// Make memory statistics show
//	#endif
	show_mem_info = 0;
	//show_cursor();
	return(0);		//presumably successful exit
}

#ifdef SATURN

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dpmi.h"

typedef struct {
	char unit;
	ushort dev_offset;
	ushort dev_segment;
} dev_list;

typedef struct _Dev_Hdr {
	unsigned int dev_next;
	unsigned short dev_att;
	ushort dev_stat;
	ushort dev_int;
	char dev_name[8];
	short dev_resv;
	char dev_letr;
	char dev_units;
} dev_header;

int find_descent_cd()
{
	dpmi_real_regs rregs;

	// Get dos memory for call...
	dev_list * buf;
	dev_header *device;
	int num_drives, i;
	unsigned cdrive, cur_drive, cdrom_drive;

	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.eax = 0x1500;
	rregs.ebx = 0;
	dpmi_real_int386x( 0x2f, &rregs );
	if ((rregs.ebx & 0xffff) == 0) {
		return -1;			// No cdrom
	}
	num_drives = rregs.ebx;

	buf = (dev_list *)dpmi_get_temp_low_buffer( sizeof(dev_list)*26 );
	if (buf==NULL) {
		return -2;			// Error getting memory!
	}

	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.es = DPMI_real_segment(buf);
	rregs.ebx = DPMI_real_offset(buf);
	rregs.eax = 0x1501;
	dpmi_real_int386x( 0x2f, &rregs );
	cdrom_drive = 0;
	_dos_getdrive(&cdrive);
	for (i = 0; i < num_drives; i++) {
		device = (dev_header *)((buf[i].dev_segment<<4)+ buf[i].dev_offset);
		_dos_setdrive(device->dev_letr,&cur_drive);
		_dos_getdrive(&cur_drive);
		if (cur_drive == device->dev_letr) {
			if (!chdir("\\descent")) {
				FILE * fp;
				fp = fopen( "saturn.hog", "rb" );
				if ( fp )	{
					cdrom_drive = device->dev_letr;
					fclose(fp);
					break;
				}
			}
		}
	}
	_dos_setdrive(cdrive,&cur_drive);
	return cdrom_drive;
}

#endif

