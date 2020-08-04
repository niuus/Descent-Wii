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
#include <ctype.h>
#include <string.h>

#include "mem.h"

// string compare without regard to case

int stricmp( const char *s1, const char *s2 )
{
	while( *s1 && *s2 )	{
		int i = *s1, j = *s2;
		if (tolower(i) != tolower(j))	return 1;
		s1++;
		s2++;
	}
	if ( *s1 || *s2 ) return 1;
	return 0;
}

int strnicmp( const char *s1, const char *s2, size_t n )
{
	while( *s1 && *s2 && n)	{
		int i = *s1, j = *s2;
		if (tolower(i) != tolower(j) )	return 1;
		s1++;
		s2++;
		n--;
	}
	return 0;
}

char* strlwr( char *s1 )
{
	while( *s1 ) {
		int i = *s1;
		*s1 = tolower(i);
		s1++;
	}
	return s1;
}

void strrev( char *s1 )
{
	int i,l;
	char *s2;

	s2 = (char *)mymalloc(strlen(s1) + 1);
	strcpy(s2, s1);
	l = strlen(s2);
	for (i = 0; i < l; i++)
		s1[l-1-i] = s2[i];
	myfree(s2);
}

