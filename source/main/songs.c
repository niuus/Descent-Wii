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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "error.h"
#include "dtypes.h"
#include "songs.h"
#include "mono.h"
#include "cfile.h"
#include "digi.h"

song_info Songs[MAX_SONGS];
int Songs_initialized = 0;

static const char* mp3_names[MAX_SONGS] = {
	"02_descent.mp3", // title
	"01_descent.mp3", // briefing
	"27_descent.mp3", // endlevel
	"26_descent.mp3", // endgame
	"25_descent.mp3", // credits
	"03_descent.mp3", // 1
	"09_descent.mp3", // 2
	"15_descent.mp3", // 3
	"05_descent.mp3", // 4
	"07_descent.mp3", // 5
	"11_descent.mp3", // 6
	"13_descent.mp3", // 7
	"17_descent.mp3", // 8
	"19_descent.mp3", // 9
	"21_descent.mp3", // 10
	"23_descent.mp3", // 11
	"04_descent.mp3", // 12
	"06_descent.mp3", // 13
	"08_descent.mp3", // 14
	"10_descent.mp3", // 15
	"12_descent.mp3", // 16
	"14_descent.mp3", // 17
	"16_descent.mp3", // 18
	"18_descent.mp3", // 19
	"20_descent.mp3", // 20
	"22_descent.mp3", // 21
	"24_descent.mp3"  // 22
};

void songs_init()
{
	int i;
	char inputline[80+1];
	CFILE * fp;

	if ( Songs_initialized ) return;

	fp = cfopen( "descent.sng", "rb" );
	if ( fp == NULL )	{
		Error( "Couldn't open descent.sng" );
	}

	i = 0;
	while (cfgets(inputline, 80, fp )) {
		char *p = strchr(inputline,'\n');
		if (p) *p = '\0';
		if ( strlen( inputline ) )	{
			struct stat st;
			Assert( i < MAX_SONGS );
			sscanf( inputline, "%s %s %s", Songs[i].filename, Songs[i].melodic_bank_file, Songs[i].drum_bank_file );
			if (mp3_names[i]) {
				strcpy(inputline, "music/");
				strcat(inputline, mp3_names[i]);
				if (stat(inputline, &st)!=-1 && !(st.st_mode&S_IFDIR))
					strcpy(Songs[i].filename, inputline);
			}
			//printf( "%d. '%s' '%s' '%s'\n",i,  Songs[i].filename, Songs[i].melodic_bank_file, Songs[i].drum_bank_file );
			i++;
		}
	}
	cfclose(fp);
	Songs_initialized = 1;
}

void songs_play_song(int songnum, int repeat)
{
	if ( !Songs_initialized ) songs_init();
	digi_play_midi_song(Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, repeat);
}

void songs_play_level_song(int levelnum)
{
	int songnum;

	Assert( levelnum != 0 );

	if ( !Songs_initialized ) songs_init();

	if (levelnum < 0)
		songnum = (-levelnum) % NUM_GAME_SONGS;
	else
		songnum = (levelnum-1) % NUM_GAME_SONGS;

	songnum += SONG_LEVEL_MUSIC;
	digi_play_midi_song(Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, 1);
}

