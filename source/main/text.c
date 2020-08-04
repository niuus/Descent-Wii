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
#include <string.h>
//#include <io.h>

//#include "cfile.h"
#include "cfile.h"
#include "mem.h"
#include "error.h"

#include "inferno.h"
#include "text.h"
#include "args.h"
#include "compbit.h"

const char *TXT_CONTROL_WIIMOTE[] = {
	"WIIMOTE",
	"NUNCHUK",
	"CLASSIC CONTROLLER"
};

char *text;

char *Text_string[N_TEXT_STRINGS+2];

void free_text()
{
	myfree(text);
}

// rotates a byte left one bit, preserving the bit falling off the right
void
encode_rotate_left(char *c)
{
	int found;

	found = 0;
	if (*c & 0x80)
		found = 1;
	*c = *c << 1;
	if (found)
		*c |= 0x01;
}

//load all the text strings for Descent
void load_text()
{
	CFILE  *tfile;
	CFILE *ifile;
	int len,i, have_binary = 0;
	char *tptr, *p;
	char *filename="descent.tex";

	if ((i=FindArg("-text"))!=0)
		filename = Args[i+1];

	if ((tfile = cfopen(filename,"rb")) == NULL) {
		filename="descent.txb";
		if ((ifile = cfopen(filename, "rb")) == NULL)
			Error("Cannot open file DESCENT.TEX or DESCENT.TXB. Is DESCENT.HOG missing?");
		have_binary = 1;

		len = cfilelength(ifile);

		MALLOC(text,char,len);

		atexit(free_text);

		cfread(text,1,len,ifile);

		cfclose(ifile);

	} else {
		int c;

		len = cfilelength(tfile);

		MALLOC(text,char,len);

		atexit(free_text);

		//fread(text,1,len,tfile);
		p = text;
		do {
			c = cfgetc( tfile );
			if ( c != 13 )
				*p++ = c;
		} while ( c!=EOF );

		cfclose(tfile);
	}

	for (i=0,tptr=text;i<N_TEXT_STRINGS;i++) {
		Text_string[i] = tptr;

		tptr = strchr(tptr,0x0a);

		if (!tptr)
			Error("Not enough strings in text file - expecting %d, found %d.\n\tPlease make sure your copy of Descent is v1.4a or later.",N_TEXT_STRINGS,i);

		if ( tptr ) *tptr++ = 0;

		if (have_binary) {
			for (p=Text_string[i]; p < tptr - 1; p++) {
				encode_rotate_left(p);
				*p = *p ^ BITMAP_TBL_XOR;
				encode_rotate_left(p);
			}
		}

		//scan for special chars (like \n)
		for (p=Text_string[i];(p=strchr(p,'\\'));p++) {
			char newchar;

			if (p[1] == 'n') newchar = '\n';
			else if (p[1] == 't') newchar = '\t';
			else if (p[1] == '\\') newchar = '\\';
			else {
				Error("Unsupported key sequence <\\%c> on line %d of file <%s>",p[1],i+1,filename);
				return;
			}

			p[0] = newchar;
			strcpy(p+1,p+2);
		}

	}

	Text_string[N_TEXT_STRINGS+0] = "CYCLE PRIMARY";
	Text_string[N_TEXT_STRINGS+1] = "CYCLE SECONDARY";
	TXT_PRESS_CTRL_R = "PRESS <+> TO RESET";
	TXT_KCONFIG_STRING_1 = "<A> changes, <-> deletes, <+> resets defaults, <Home> exits";
	p = strstr(TXT_SELECT_DEMO, "<Ctrl-D>");
	if (p)
		strcpy(p, "<-> deletes");
	p = strstr(TXT_SELECT_PILOT, "<Ctrl-D>");
	if (p)
		strcpy(p, "<-> deletes");

//	Assert(tptr==text+len || tptr==text+len-2);

}


