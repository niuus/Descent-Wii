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

#ifndef _CFILE_H
#define _CFILE_H

#include <stdio.h>

typedef struct CFILE {
	FILE 				*file;
	int				size;
	int				lib_offset;
	int				raw_position;
} CFILE;

CFILE * cfopen(const char * filename, char * mode);
int cfilelength( CFILE *fp );							// Returns actual size of file...
size_t cfread( void * buf, size_t elsize, size_t nelem, CFILE * fp );
void cfclose( CFILE * cfile );
int cfgetc( CFILE * fp );
int cfseek( CFILE *fp, long int offset, int where );
int cftell( CFILE * fp );
char * cfgets( char * buf, size_t n, CFILE * fp );

int cfexist( char * filename );	// Returns true if file exists on disk (1) or in hog (2).

// Allows files to be gotten from an alternate hog file.
// Passing NULL disables this.
void cfile_use_alternate_hogfile( char * name );


#endif
