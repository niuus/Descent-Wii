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
//
// Auto-generated include file
// Generated by MAKE from INFERNO.INI settings
//

#define NEWDEMO			//Demo is always IN

#define RELEASE			//This is NOT a release version
//#define DEMO_ONLY		//Game is playable
//#define NMONO			//NMONO is OFF
//#define _MARK_ON		//MARKS are OFF
//#define PASSWORD		//PASSWORD is OFF
//#define STORE_DEMO 1	//STORE_DEMO is OFF
#define PIGGY_USE_PAGING 1		//Don't use paging from pig file
//#define SATURN 1		// Not building for Destination Saturn (OEM version)
//#define COMPACT_SEGS 1		// Not building for Destination Saturn (OEM version)
#ifdef __wii__
#define NETWORK 1		// Include network functions
#endif
//#define SERIAL 1        // Include serial/modem functions

#ifdef RELEASE
#define NDEBUG
#endif