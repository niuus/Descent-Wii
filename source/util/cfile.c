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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "cfile.h"
#include "mem.h"
#include "byteswap.h"
#include "error.h"
#include "fileutil.h"

typedef struct hogfile {
	char	name[13];
	int	offset;
	int 	length;
} hogfile;

#define MAX_HOGFILES 250

hogfile HogFiles[MAX_HOGFILES];
char Hogfile_initialized = 0;
int Num_hogfiles = 0;

hogfile AltHogFiles[MAX_HOGFILES];
char AltHogfile_initialized = 0;
int AltNum_hogfiles = 0;
char AltHogFilename[64];

char AltHogDir[64];
char AltHogdir_initialized = 0;

void cfile_use_alternate_hogdir( char * path )
{
	if ( path )	{
		strcpy( AltHogDir, path );
		AltHogdir_initialized = 1;
	} else {
		AltHogdir_initialized = 0;
	}
}

extern int descent_critical_error;

FILE * cfile_get_filehandle( char * filename, char * mode )
{
	FILE * fp;
	char temp[128];

	descent_critical_error = 0;
	fp = fopen( filename, mode );
	if ( fp && descent_critical_error )	{
		fclose(fp);
		fp = NULL;
	}
	if ( (fp==NULL) && (AltHogdir_initialized) )	{
		strcpy( temp, AltHogDir );
		strcat( temp, filename );
		descent_critical_error = 0;
		fp = fopen( temp, mode );
		if ( fp && descent_critical_error )	{
			fclose(fp);
			fp = NULL;
		}
	}
	return fp;
}

void cfile_init_hogfile(char *fname, hogfile * hog_files, int * nfiles )
{
	char id[4];
	FILE * fp;
	int i, len;

	*nfiles = 0;

	fp = cfile_get_filehandle( fname, "rb" );
	if ( fp == NULL ) return;

	fread( id, 3, 1, fp );
	if ( strncmp( id, "DHF", 3 ) )	{
		fclose(fp);
		return;
	}

	while( 1 )
	{
		if ( *nfiles >= MAX_HOGFILES ) {
			Warning( "ERROR: HOGFILE IS LIMITED TO %d FILES\n",  MAX_HOGFILES );
			fclose(fp);
			exit(1);
		}
		i = fread( hog_files[*nfiles].name, 13, 1, fp );
		if ( i != 1 )	{
			fclose(fp);
			return;
		}
		i = fread( &len, 4, 1, fp );
		if ( i != 1 )	{
			fclose(fp);
			return;
		}
		hog_files[*nfiles].length = swapint(len);
		if (hog_files[*nfiles].length < 0)
			Warning ("Hogfile length < 0");
		hog_files[*nfiles].offset = ftell( fp );
		*nfiles = (*nfiles) + 1;
		// Skip over
		i = fseek( fp, swapint(len), SEEK_CUR );
	}
	fclose(fp);
}

FILE * cfile_find_libfile(char * name, int * length)
{
	FILE * fp;
	int i;

	if ( AltHogfile_initialized )	{
		for (i=0; i<AltNum_hogfiles; i++ )	{
			if ( !stricmp( AltHogFiles[i].name, name ))	{
				fp = cfile_get_filehandle( AltHogFilename, "rb" );
				if ( fp == NULL ) return NULL;
				fseek( fp,  AltHogFiles[i].offset, SEEK_SET );
				*length = AltHogFiles[i].length;
				return fp;
			}
		}
	}

	if ( !Hogfile_initialized ) 	{
		cfile_init_hogfile( "descent.hog", HogFiles, &Num_hogfiles );
		Hogfile_initialized = 1;
	}

	for (i=0; i<Num_hogfiles; i++ )	{
		if ( !stricmp( HogFiles[i].name, name ))	{
			fp = cfile_get_filehandle( "descent.hog", "rb" );
			if (fp) {
				fseek( fp,  HogFiles[i].offset, SEEK_SET );
				*length = HogFiles[i].length;
			}
			return fp;
		}
	}
	return NULL;
}

void cfile_use_alternate_hogfile( char * name )
{
	if ( name )	{
		strcpy( AltHogFilename, name );
		cfile_init_hogfile( AltHogFilename, AltHogFiles, &AltNum_hogfiles );
		AltHogfile_initialized = 1;
	} else {
		AltHogfile_initialized = 0;
	}
}

int cfexist( char * filename )
{
	int length;
	FILE *fp;

	fp = cfile_get_filehandle( filename, "rb" );		// Check for non-hog file first...
	if ( fp )	{
		fclose(fp);
		return 1;
	}

	fp = cfile_find_libfile(filename, &length );
	if ( fp )	{
		fclose(fp);
		return 2;		// file found in hog
	}

	return 0;		// Couldn't find it.
}


CFILE * cfopen(const char * filename, char * mode )
{
	int length;
	FILE * fp;
	CFILE *cfile;
	char new_filename[256], *p;

	if (stricmp( mode, "rb"))	{
		Warning( "CFILES CAN ONLY BE OPENED WITH RB\n" );
		exit(1);
	}

	strcpy(new_filename, filename);
	while ( (p = strchr(new_filename, 13) ) )
		*p = '\0';

	while ( (p = strchr(new_filename, 10) ) )
		*p = '\0';

	fp = cfile_get_filehandle( new_filename, mode );		// Check for non-hog file first...
	if ( !fp ) {
		fp = cfile_find_libfile(new_filename, &length );
		if ( !fp )
			return NULL;		// No file found
		cfile = (CFILE *)mymalloc ( sizeof(CFILE) );
		if ( cfile == NULL ) {
			fclose(fp);
			return NULL;
		}
		cfile->file = fp;
		cfile->size = length;
		cfile->lib_offset = ftell( fp );
		cfile->raw_position = 0;
	} else {
		cfile = (CFILE *)mymalloc ( sizeof(CFILE) );
		if ( cfile == NULL ) {
			fclose(fp);
			return NULL;
		}
		cfile->file = fp;
		cfile->size = filelength( fileno(fp) );
		cfile->lib_offset = 0;
		cfile->raw_position = 0;
	}

	return cfile;
}

int cfilelength( CFILE *fp )
{
	return fp->size;
}

int cfgetc( CFILE * fp )
{
	int c;

	if (fp->raw_position >= fp->size ) return EOF;

	c = getc( fp->file );
	if (c!=EOF)
		fp->raw_position++;

//	Assert( fp->raw_position==(ftell(fp->file)-fp->lib_offset) );

	return c;
}

char * cfgets( char * buf, size_t n, CFILE * fp )
{
	char * t = buf;
	int i;
	int c;

	for (i=0; i<n-1; i++ )	{
		do {
			if (fp->raw_position >= fp->size ) {
				*buf = 0;
				return NULL;
			}
			c = fgetc( fp->file );
			fp->raw_position++;
		} while ( c == 13 );
		*buf++ = c;
		if ( c == 10 )
			c = '\n';
		if ( c=='\n' ) break;
	}
	*buf++ = 0;
	return  t;
}

size_t cfread( void * buf, size_t elsize, size_t nelem, CFILE * fp )
{
	int i;
	if ((fp->raw_position+(elsize*nelem)) > fp->size ) return EOF;
	i = fread( buf, elsize, nelem, fp->file );
	fp->raw_position += i*elsize;
	return i;
}


int cftell( CFILE *fp )
{
	return fp->raw_position;
}

int cfseek( CFILE *fp, long int offset, int where )
{
	int c, goal_position;

	switch( where )	{
	case SEEK_SET:
		goal_position = offset;
		break;
	case SEEK_CUR:
		goal_position = fp->raw_position+offset;
		break;
	case SEEK_END:
		goal_position = fp->size+offset;
		break;
	default:
		return 1;
	}
	c = fseek( fp->file, fp->lib_offset + goal_position, SEEK_SET );
	fp->raw_position = ftell(fp->file)-fp->lib_offset;
	return c;
}

void cfclose( CFILE * fp )
{
	fclose(fp->file);
	myfree(fp);
	return;
}



