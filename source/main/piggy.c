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
#include <stdarg.h>
#include <string.h>
#include <malloc.h> // memalign

#include "dtypes.h"
#include "inferno.h"
#include "gr.h"
#include "mem.h"
#include "mono.h"
#include "error.h"
#include "sounds.h"
#include "bm.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "rle.h"
#include "screens.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "cfile.h"
#include "newmenu.h"
#include "fileutil.h"

#include "input.h"
#include "wiisys.h"

//#define NO_DUMP_SOUNDS	1		//if set, dump bitmaps but not sounds

ubyte *BitmapBits = NULL;
ubyte *SoundBits = NULL;

typedef struct BitmapFile	{
	char			name[15];
} BitmapFile;

typedef struct SoundFile	{
	char			name[15];
} SoundFile;

hashtable AllBitmapsNames;
hashtable AllDigiSndNames;

int Num_bitmap_files = 0;
int Num_sound_files = 0;

digi_sound GameSounds[MAX_SOUND_FILES];
int SoundOffset[MAX_SOUND_FILES];
grs_bitmap *GameBitmaps = NULL;

int Num_bitmap_files_new = 0;
int Num_sound_files_new = 0;
static BitmapFile *AllBitmaps = NULL;
static SoundFile *AllSounds = NULL;

int piggy_low_memory = 0;

#define DBM_FLAG_LARGE 	128		// Flags added onto the flags struct in b
#define DBM_FLAG_ABM		64
#define RLE_CODE 			0xE0
#define NOT_RLE_CODE		31

char new_baldguy_pcx[] = "btexture.xxx";
int Piggy_bitmap_cache_size = 0;
int Piggy_bitmap_cache_next = 0;
ubyte * Piggy_bitmap_cache_data = NULL;
static int GameBitmapOffset[MAX_BITMAP_FILES];
static ubyte GameBitmapFlags[MAX_BITMAP_FILES];
static ushort GameBitmapXlat[MAX_BITMAP_FILES];
#ifndef MAC_SHAREWARE
static ubyte bald_guy_cheat_index = 0;
#endif

#ifdef MAC_SHAREWARE
#define PIGGY_BUFFER_SIZE		(1536*1024)
#define LOW_PIGGY_BUFFER_SIZE	(1280*1024)
#else
#define PIGGY_BUFFER_SIZE		(2048*1024)
#define LOW_PIGGY_BUFFER_SIZE	(1536*1024)
#endif

int piggy_page_flushed = 0;

typedef struct DiskBitmapHeader {
	char 	name[8];
	ubyte 	dflags;
	ubyte	width;
	ubyte	height;
	ubyte	flags;
	ubyte	avg_color;
	int		offset;
} DiskBitmapHeader;

typedef struct DiskSoundHeader {
	char	name[8];
	int		length;
	int		data_length;
	int 	offset;
} DiskSoundHeader;

ubyte BigPig = 0;


//#define BUILD_PSX_DATA 1

#ifdef BUILD_PSX_DATA
FILE * count_file = NULL;

int num_good=0,num_bad=0;
int num_good64=0,num_bad64=0;

void close_count_file()
{
 	if ( count_file )	{
		fprintf( count_file,"Good = %d\n", num_good );
		fprintf( count_file,"Bad = %d\n", num_bad );
		fprintf( count_file,"64Good = %d\n", num_good64 );
		fprintf( count_file,"64Bad = %d\n", num_bad64 );
		fclose(count_file);
		count_file=NULL;
	}
}

void count_colors( int bnum, grs_bitmap * bmp )
{
	int i,colors;
	ushort n[256];

	varquant( bnum, bmp );

	if ( count_file == NULL )	{
		atexit( close_count_file );
		count_file = fopen( "bitmap.cnt", "wt" );
	}
	for (i=0; i<256; i++ )
		n[i] = 0;

	for (i=0; i<bmp->bm_w*bmp->bm_h; i++ )	{
		n[bmp->bm_data[i]]++;
	}

	colors = 0;
	for (i=0; i<256; i++ )
		if (n[i]) colors++;

	if ( colors > 16 )	{
		if ( (bmp->bm_w==64) && (bmp->bm_h==64) ) num_bad64++;
		num_bad++;
		fprintf( count_file, "Bmp has %d colors (%d x %d)\n", colors, bmp->bm_w, bmp->bm_h );
	} else {
		if ( (bmp->bm_w==64) && (bmp->bm_h==64) ) num_good64++;
		num_good++;
	}
}
#endif

bitmap_index piggy_register_bitmap( grs_bitmap * bmp, char * name, int in_file )
{
	bitmap_index temp;
	Assert( Num_bitmap_files < MAX_BITMAP_FILES );

	temp.index = Num_bitmap_files;


	if (!in_file)	{
		if ( !BigPig )	gr_bitmap_rle_compress( bmp );
		Num_bitmap_files_new++;
	}

	strncpy( AllBitmaps[Num_bitmap_files].name, name, 12 );
	hashtable_insert( &AllBitmapsNames, AllBitmaps[Num_bitmap_files].name, Num_bitmap_files );
	GameBitmaps[Num_bitmap_files] = *bmp;
	if ( !in_file )	{
		GameBitmapOffset[Num_bitmap_files] = 0;
		GameBitmapFlags[Num_bitmap_files] = bmp->bm_flags;
	}
	Num_bitmap_files++;

	return temp;
}

int piggy_register_sound( digi_sound * snd, char * name, int in_file )
{
	int i;

	Assert( Num_sound_files < MAX_SOUND_FILES );

	strncpy( AllSounds[Num_sound_files].name, name, 12 );
	hashtable_insert( &AllDigiSndNames, AllSounds[Num_sound_files].name, Num_sound_files );
	GameSounds[Num_sound_files] = *snd;
	if ( !in_file )	{
		SoundOffset[Num_sound_files] = 0;
	}

	i = Num_sound_files;

	if (!in_file)
		Num_sound_files_new++;

	Num_sound_files++;
	return i;
}

bitmap_index piggy_find_bitmap( char * name )
{
	bitmap_index bmp;
	int i;

	bmp.index = 0;

	i = hashtable_search( &AllBitmapsNames, name );
	Assert( i != 0 );
	if ( i < 0 )
		return bmp;

	bmp.index = i;
	return bmp;
}

int piggy_find_sound( char * name )
{
	int i;

	i = hashtable_search( &AllDigiSndNames, name );

	if ( i < 0 )
		return 255;

	return i;
}

CFILE * Piggy_fp = NULL;

void piggy_close_file()
{
//	if ( Piggy_fp )	{
//		cfclose( Piggy_fp );
//		Piggy_fp	= NULL;
//	}
}

ubyte bogus_data[64*64];
grs_bitmap bogus_bitmap;
ubyte bogus_bitmap_initialized=0;
digi_sound bogus_sound;

#ifndef MAC_SHAREWARE
#define	BALD_GUY_CHEAT_SIZE	7

ubyte	bald_guy_cheat_1[BALD_GUY_CHEAT_SIZE] = { KEY_B ^ 0xF0 ^ 0xab,
												KEY_A ^ 0xE0 ^ 0xab,
												KEY_L ^ 0xD0 ^ 0xab,
												KEY_D ^ 0xC0 ^ 0xab,
												KEY_G ^ 0xB0 ^ 0xab,
												KEY_U ^ 0xA0 ^ 0xab,
												KEY_Y ^ 0x90 ^ 0xab };
#endif

extern void bm_read_all(CFILE * fp);

int piggy_init()
{
	int sbytes = 0;
	char temp_name_read[16];
	char temp_name[16];
	grs_bitmap temp_bitmap;
	digi_sound temp_sound;
	DiskBitmapHeader bmh;
	DiskSoundHeader sndh;
	int header_size, N_bitmaps, N_sounds;
	int i,size;
	char * filename;
	int Pigdata_start;

	hashtable_init( &AllBitmapsNames, MAX_BITMAP_FILES );
	hashtable_init( &AllDigiSndNames, MAX_SOUND_FILES );

	if (GameBitmaps == NULL) {
		GameBitmaps = (grs_bitmap *)mymalloc(sizeof(grs_bitmap) * MAX_BITMAP_FILES);
		if (GameBitmaps == NULL)
			Error("Cannot allocate space for game bitmaps in piggy.c");
	}

	if (AllBitmaps == NULL) {
		AllBitmaps = (BitmapFile *)mymalloc(sizeof(BitmapFile) * MAX_BITMAP_FILES);
		if (AllBitmaps == NULL)
			Error("Cannot allocate space for bitmap filenames in piggy.c");
	}

	if (AllSounds == NULL) {
		AllSounds = (SoundFile *)mymalloc(sizeof(SoundFile) * MAX_SOUND_FILES);
		if (AllSounds == NULL)
			Error("Cannot allocate space for sound filenames in piggy.c");
	}

	for (i=0; i<MAX_SOUND_FILES; i++ )	{
		GameSounds[i].length = 0;
		GameSounds[i].data = NULL;
		SoundOffset[i] = 0;
	}

	for (i=0; i<MAX_BITMAP_FILES; i++ )
		GameBitmapXlat[i] = i;

	if ( !bogus_bitmap_initialized )	{
		int i;
		ubyte c;
		bogus_bitmap_initialized = 1;
		memset( &bogus_bitmap, 0, sizeof(grs_bitmap) );
		bogus_bitmap.bm_w = bogus_bitmap.bm_h = bogus_bitmap.bm_rowsize = 64;
		bogus_bitmap.bm_data = bogus_data;
		c = gr_find_closest_color( 0, 0, 63 );
		for (i=0; i<4096; i++ ) bogus_data[i] = c;
		c = gr_find_closest_color( 63, 0, 0 );
		// Make a big red X !
		for (i=0; i<64; i++ )	{
			bogus_data[i*64+i] = c;
			bogus_data[i*64+(63-i)] = c;
		}
		piggy_register_bitmap( &bogus_bitmap, "bogus", 1 );
		bogus_sound.length = 64*64;
		bogus_sound.data = bogus_data;
		GameBitmapOffset[0] = 0;
	}

	filename = "DESCENT.PIG";

	if ( FindArg( "-bigpig" ))
		BigPig = 1;

	if ( FindArg( "-lowmem" ))
		piggy_low_memory = 1;

	if ( FindArg( "-nolowmem" ))
		piggy_low_memory = 0;

	if (use_alt_textures)
		piggy_low_memory = 1;

//MWA	if (piggy_low_memory)
//MWA		digi_lomem = 1;

	if ( (i=FindArg( "-piggy" )) )	{
		filename	= Args[i+1];
		mprintf( (0, "Using alternate pigfile, '%s'\n", filename ));
	}
	Piggy_fp = cfopen( filename, "rb" );
	if (Piggy_fp==NULL) return 0;

	Pigdata_start = read_int_swap(Piggy_fp);
#ifdef EDITOR
	if ( FindArg("-nobm") )
#endif
	{
		bm_read_all( Piggy_fp );	// Note connection to above if!!!
		for (i = 0; i < MAX_BITMAP_FILES; i++)
			GameBitmapXlat[i] = read_short_swap(Piggy_fp);
		digi_load_sounds();
	}

	cfseek( Piggy_fp, Pigdata_start, SEEK_SET );
	size = cfilelength(Piggy_fp) - Pigdata_start;
	mprintf( (0, "\nReading data (%d KB) ", size/1024 ));

	N_bitmaps = read_int_swap(Piggy_fp);
	size -= sizeof(int);
	N_sounds = read_int_swap(Piggy_fp);
	size -= sizeof(int);

//	header_size = (N_bitmaps*sizeof(DiskBitmapHeader)) + (N_sounds*sizeof(DiskSoundHeader));
	header_size = ((N_bitmaps*17) + (N_sounds*20));

	gr_set_curfont( Gamefonts[GFONT_SMALL] );
	gr_set_fontcolor(gr_find_closest_color_current( 20, 20, 20 ),-1 );
	gr_printf( 0x8000, 179, "%s...", TXT_LOADING_DATA );
	bitblt_to_screen();

	for (i=0; i<N_bitmaps; i++ )	{
		cfread(bmh.name, 8, 1, Piggy_fp);
		bmh.dflags = read_byte(Piggy_fp);
		bmh.width = read_byte(Piggy_fp);
		bmh.height = read_byte(Piggy_fp);
		bmh.flags = read_byte(Piggy_fp);
		bmh.avg_color = read_byte(Piggy_fp);
		bmh.offset = read_int_swap(Piggy_fp);

		//size -= sizeof(DiskBitmapHeader);
		memcpy( temp_name_read, bmh.name, 8 );
		temp_name_read[8] = 0;
		if ( bmh.dflags & DBM_FLAG_ABM )
			sprintf( temp_name, "%s#%d", temp_name_read, bmh.dflags & 63 );
		else
			strcpy( temp_name, temp_name_read );
		memset( &temp_bitmap, 0, sizeof(grs_bitmap) );
		if ( bmh.dflags & DBM_FLAG_LARGE )
			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width+256;
		else
			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width;
		temp_bitmap.bm_h = bmh.height;
		temp_bitmap.bm_flags = BM_FLAG_PAGED_OUT;
		temp_bitmap.avg_color = bmh.avg_color;
		temp_bitmap.bm_data = Piggy_bitmap_cache_data;

#if 0 // get rid of this stupid hack, lying about the resources will not end well
// HACK HACK HACK!!!!!
		if (!strnicmp(bmh.name, "cockpit", 7) || !strnicmp(bmh.name, "status", 6) || !strnicmp(bmh.name, "rearview", 8)) {
			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = 640;
		}
		if (!strnicmp(bmh.name, "cockpit", 7) || !strnicmp(bmh.name, "rearview", 8))
			temp_bitmap.bm_h = 480;
#endif

		GameBitmapFlags[i+1] = 0;
		if ( bmh.flags & BM_FLAG_TRANSPARENT ) GameBitmapFlags[i+1] |= BM_FLAG_TRANSPARENT;
		if ( bmh.flags & BM_FLAG_SUPER_TRANSPARENT ) GameBitmapFlags[i+1] |= BM_FLAG_SUPER_TRANSPARENT;
		if ( bmh.flags & BM_FLAG_NO_LIGHTING ) GameBitmapFlags[i+1] |= BM_FLAG_NO_LIGHTING;
		if ( bmh.flags & BM_FLAG_RLE ) GameBitmapFlags[i+1] |= BM_FLAG_RLE;

		GameBitmapOffset[i+1] = bmh.offset + header_size + (sizeof(int)*2) + Pigdata_start;
		Assert( (i+1) == Num_bitmap_files );
		piggy_register_bitmap( &temp_bitmap, temp_name, 1 );
	}

	for (i=0; i<N_sounds; i++ )	{
		cfread(sndh.name, 8, 1, Piggy_fp);
		sndh.length = read_int_swap(Piggy_fp);
		sndh.data_length = read_int_swap(Piggy_fp);
		sndh.offset = read_int_swap(Piggy_fp);

		//size -= sizeof(DiskSoundHeader);
		temp_sound.length = sndh.length;
		temp_sound.data = (ubyte *)(sndh.offset + header_size + (sizeof(int)*2)+Pigdata_start);
		SoundOffset[Num_sound_files] = sndh.offset + header_size + (sizeof(int)*2)+Pigdata_start;
		memcpy( temp_name_read, sndh.name, 8 );
		temp_name_read[8] = 0;
		piggy_register_sound( &temp_sound, temp_name_read, 1 );
		// sound storage should be aligned to 32-bytes and length rounded up
		sbytes += (sndh.length+31)&~31;
		//mprintf(( 0, "%d bytes of sound\n", sbytes ));
	}

	SoundBits = memalign(32, sbytes + 16 );
	if ( SoundBits == NULL )
		Error( "Not enough memory to load DESCENT.PIG sounds\n" );

#ifdef EDITOR
	Piggy_bitmap_cache_size	= size - header_size - sbytes + 16;
	Assert( Piggy_bitmap_cache_size > 0 );
#else
	if (!piggy_low_memory)
		Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
	else
		Piggy_bitmap_cache_size = LOW_PIGGY_BUFFER_SIZE;
#endif
	BitmapBits = mymalloc( Piggy_bitmap_cache_size );
	if ( BitmapBits == NULL )
		Error( "Not enough memory to load DESCENT.PIG bitmaps\n" );
	Piggy_bitmap_cache_data = BitmapBits;
	Piggy_bitmap_cache_next = 0;

	mprintf(( 0, "\nBitmaps: %d KB   Sounds: %d KB\n", Piggy_bitmap_cache_size/1024, sbytes/1024 ));

	atexit(piggy_close_file);

//	mprintf( (0, "<<<<Paging in all piggy bitmaps...>>>>>" ));
//	for (i=0; i < Num_bitmap_files; i++ )	{
//		bitmap_index bi;
//		bi.index = i;
//		PIGGY_PAGE_IN( bi );
//	}
//	mprintf( (0, "\n (Used %d / %d KB)\n", Piggy_bitmap_cache_next/1024, (size - header_size - sbytes + 16)/1024 ));
//	key_getch();

	return 0;
}

int piggy_is_needed(int soundnum)
{
	int i;

	if ( !digi_lomem ) return 1;

	for (i=0; i<MAX_SOUNDS; i++ )	{
		if ( (AltSounds[i] < 255) && (Sounds[AltSounds[i]] == soundnum) )
			return 1;
	}
	return 0;
}

void piggy_read_sounds()
{
	ubyte * ptr;
	int i, sbytes;

	ptr = SoundBits;
	sbytes = 0;

	for (i=0; i<Num_sound_files; i++ )	{
		digi_sound *snd = &GameSounds[i];

		if ( SoundOffset[i] > 0 )	{
			if ( piggy_is_needed(i) )	{
				cfseek( Piggy_fp, SoundOffset[i], SEEK_SET );

				// Read in the sound data!!!
				snd->data = ptr;
				// align for wii
				ptr += (snd->length+31)&~31;
				sbytes += snd->length;
				cfread(snd->data, snd->length, 1, Piggy_fp);
			}
		}
	}

	mprintf(( 0, "\nActual Sound usage: %d KB\n", sbytes/1024 ));
}

extern int descent_critical_error;
extern unsigned descent_critical_deverror;
extern unsigned descent_critical_errcode;

char * crit_errors[13] = { "Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error", \
"Bad struct length", "Seek Error", "Unknown media type", "Sector not found", "Printer out of paper", "Write Fault", \
"Read fault", "General Failure" };

void piggy_critical_error()
{
	grs_canvas * save_canv;
	grs_font * save_font;
	int i;
	save_canv = grd_curcanv;
	save_font = grd_curcanv->cv_font;
	gr_palette_load( gr_palette );
	i = nm_messagebox( "Disk Error", 2, "Retry", "Exit", "%s\non drive %c:", crit_errors[descent_critical_errcode&0xf], (descent_critical_deverror&0xf)+'A'  );
	if ( i == 1 )
		exit(1);
	gr_set_current_canvas(save_canv);
	grd_curcanv->cv_font = save_font;
}

void piggy_bitmap_page_in( bitmap_index bitmap )
{
	grs_bitmap * bmp;
	int i,org_i=-1;

	i = bitmap.index;
	Assert( i >= 0 );
	Assert( i < MAX_BITMAP_FILES );
	Assert( i < Num_bitmap_files );
	Assert( Piggy_bitmap_cache_size > 0 );

	if ( GameBitmapOffset[i] == 0 ) return;		// A read-from-disk bitmap!!!

	if (piggy_low_memory) {
		org_i = i;
		i = GameBitmapXlat[i];		// Xlat for low-memory settings!
	}
	bmp = &GameBitmaps[i];

	if ( bmp->bm_flags & BM_FLAG_PAGED_OUT )	{
		stop_time();

	ReDoIt:
		descent_critical_error = 0;
		cfseek( Piggy_fp, GameBitmapOffset[i], SEEK_SET );
		if ( descent_critical_error )	{
			piggy_critical_error();
			goto ReDoIt;
		}

		bmp->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
		bmp->bm_flags = GameBitmapFlags[i];

		if ( bmp->bm_flags & BM_FLAG_RLE )	{
			int zsize = 0;
			descent_critical_error = 0;
			zsize = read_int_swap(Piggy_fp);
			if ( descent_critical_error )	{
				piggy_critical_error();
				goto ReDoIt;
			}

			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+zsize < Piggy_bitmap_cache_size );
#ifdef PIGGY_USE_PAGING
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size )	{
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
#endif
			memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], &zsize, sizeof(int) );
			Piggy_bitmap_cache_next += sizeof(int);
			descent_critical_error = 0;
			cfread( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, zsize-4, Piggy_fp );
			if ( descent_critical_error )	{
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next += zsize-4;
		} else {
			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) < Piggy_bitmap_cache_size );
#ifdef PIGGY_USE_PAGING
			if ( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) >= Piggy_bitmap_cache_size )	{
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
#endif
			descent_critical_error = 0;
			cfread( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, bmp->bm_h*bmp->bm_w, Piggy_fp );
			if ( descent_critical_error )	{
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next+=bmp->bm_h*bmp->bm_w;
		}
		start_time();
	}

	if (piggy_low_memory) {
		if ( org_i != i )
			GameBitmaps[org_i] = GameBitmaps[i];
	}
}

void piggy_bitmap_page_out_all()
{
	int i;

	Piggy_bitmap_cache_next = 0;

	piggy_page_flushed++;

	texmerge_flush();
	rle_cache_flush();

	for (i=0; i<Num_bitmap_files; i++ )		{
		if ( GameBitmapOffset[i] > 0 )	{	// Don't page out bitmaps read from disk!!!
			GameBitmaps[i].bm_flags = BM_FLAG_PAGED_OUT;
			GameBitmaps[i].bm_data = Piggy_bitmap_cache_data;
		}
	}

	mprintf(( 0, "Flushing piggy bitmap cache\n" ));
}

void piggy_load_level_data()
{
	piggy_bitmap_page_out_all();
	paging_touch_all();
}

void piggy_close()
{
	if (BitmapBits)
		myfree(BitmapBits);

	if ( SoundBits )
		myfree( SoundBits );

	hashtable_free( &AllBitmapsNames );
	hashtable_free( &AllDigiSndNames );

}

int piggy_does_bitmap_exist_slow( char * name )
{
	int i;

	for (i=0; i<Num_bitmap_files; i++ )	{
		if ( !strcmp( AllBitmaps[i].name, name) )
			return 1;
	}
	return 0;
}


#define NUM_GAUGE_BITMAPS 14
char * gauge_bitmap_names[NUM_GAUGE_BITMAPS] = { "gauge01", "gauge02", "gauge06", "targ01", "targ02", "targ03", "targ04", "targ05", "targ06", "gauge18", "targ01pc", "targ02pc", "targ03pc", "gaug18pc" };

int piggy_is_gauge_bitmap( char * base_name )
{
	int i;
	for (i=0; i<NUM_GAUGE_BITMAPS; i++ )	{
		if ( !stricmp( base_name, gauge_bitmap_names[i] ))
			return 1;
	}

	return 0;
}

#ifndef MAC_SHAREWARE
extern ubyte baldguy_cheat;

void bald_guy_cheat(int key)
{
	if (key == (bald_guy_cheat_1[bald_guy_cheat_index] ^ (0xf0 - (bald_guy_cheat_index << 4)) ^ 0xab)) {
		bald_guy_cheat_index++;
		if (bald_guy_cheat_index == BALD_GUY_CHEAT_SIZE)	{
			baldguy_cheat = 1;
			bald_guy_cheat_index = 0;
		}
	} else
		bald_guy_cheat_index = 0;
}
#endif


int piggy_is_substitutable_bitmap( char * name, char * subst_name )
{
	int frame;
	char * p;
	char base_name[ 16 ];

	strcpy( subst_name, name );
	p = strchr( subst_name, '#' );
	if ( p ) 	{
		frame = atoi( &p[1] );
		*p = 0;
		strcpy( base_name, subst_name );
		if ( !piggy_is_gauge_bitmap( base_name ))	{
			sprintf( subst_name, "%s#%d", base_name, frame+1 );
			if ( piggy_does_bitmap_exist_slow( subst_name )  ) 	{
				if ( frame & 1 ) {
					sprintf( subst_name, "%s#%d", base_name, frame-1 );
					return 1;
				}
			}
		}
	}
	strcpy( subst_name, name );
	return 0;
}

