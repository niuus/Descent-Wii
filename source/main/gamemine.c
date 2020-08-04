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
#include <math.h>
#include <string.h>

#include "mono.h"

#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "error.h"
#include "gameseg.h"
#include "switch.h"

#include "game.h"
#include "newmenu.h"

#ifdef EDITOR
#include "editor\editor.h"
#endif

#include "cfile.h"
#include "fuelcen.h"

#include "hash.h"
#include "input.h"
#include "piggy.h"
#include "byteswap.h"
#include "fileutil.h"

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

struct mtfi mine_top_fileinfo;    // Should be same as first two fields below...
struct mfi mine_fileinfo;
struct mh mine_header;
struct me mine_editor;

int CreateDefaultNewSegment();
static int load_mine_data_compiled_new(CFILE *LoadFile);

#define COMPILED_MINE_VERSION 0

int	New_file_format_load = 1;

int load_mine_data_compiled(CFILE *LoadFile)
{
	int i,segnum,sidenum;
	ubyte version;
	short temp_short;
	ushort temp_ushort;

#ifndef MAC_SHAREWARE
	if (New_file_format_load)
		return load_mine_data_compiled_new(LoadFile);
#endif

	//	For compiled levels, textures map to themselves, prevent tmap_override always being gray,
	//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
	//
	// Although in a cloud of arrogant glee, he forgot to ifdef it on EDITOR!
	// (Matt told me to write that!)
#ifdef EDITOR
	for (i=0; i<MAX_TEXTURES; i++)
		tmap_xlate_table[i] = i;
#endif
//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	cfread( &version, sizeof(ubyte), 1, LoadFile );						// 1 byte = compiled version
	Assert( version==COMPILED_MINE_VERSION );
//	cfread( &Num_vertices, sizeof(int), 1, LoadFile );					// 4 bytes = Num_vertices
	Num_vertices = read_int_swap(LoadFile);
	Assert( Num_vertices <= MAX_VERTICES );
//	cfread( &Num_segments, sizeof(int), 1, LoadFile );					// 4 bytes = Num_segments
	Num_segments = read_int_swap(LoadFile);
	Assert( Num_segments <= MAX_SEGMENTS );
//	cfread( Vertices, sizeof(vms_vector), Num_vertices, LoadFile );

	for (i = 0; i < Num_vertices; i++) {
		Vertices[i].x = read_fix_swap(LoadFile);
		Vertices[i].y = read_fix_swap(LoadFile);
		Vertices[i].z = read_fix_swap(LoadFile);
	}


	for (segnum=0; segnum<Num_segments; segnum++ )	{
		#ifdef EDITOR
		Segments[segnum].segnum = segnum;
		Segments[segnum].group = 0;
		#endif

		// Read short Segments[segnum].children[MAX_SIDES_PER_SEGMENT]
// 		cfread( Segments[segnum].children, sizeof(short), MAX_SIDES_PER_SEGMENT, LoadFile );
		for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
			Segments[segnum].children[i] = read_short_swap(LoadFile);


		// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
//		cfread( Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT, LoadFile );
		for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
			Segments[segnum].verts[i] = read_short_swap(LoadFile);

		Segments[segnum].objects = -1;

		// Read ubyte	Segments[segnum].special
		cfread( &Segments[segnum].special, sizeof(ubyte), 1, LoadFile );
		// Read byte	Segments[segnum].matcen_num
		cfread( &Segments[segnum].matcen_num, sizeof(ubyte), 1, LoadFile );
		// Read short	Segments[segnum].value
//		cfread( &Segments[segnum].value, sizeof(short), 1, LoadFile );
		Segments[segnum].value = read_short_swap(LoadFile);

		// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
//		cfread( &temp_ushort, sizeof(temp_ushort), 1, LoadFile );
		temp_ushort = read_short_swap(LoadFile);
		Segments[segnum].static_light	= ((fix)temp_ushort) << 4;
		//cfread( &Segments[segnum].static_light, sizeof(fix), 1, LoadFile );

		// Read the walls as a 6 byte array
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			ubyte byte_wallnum;
			Segments[segnum].sides[sidenum].pad = 0;
			cfread( &byte_wallnum, sizeof(ubyte), 1, LoadFile );
			if ( byte_wallnum == 255 )
				Segments[segnum].sides[sidenum].wall_num = -1;
			else
				Segments[segnum].sides[sidenum].wall_num = byte_wallnum;
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{

			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				// Read short Segments[segnum].sides[sidenum].tmap_num;
//				cfread( &Segments[segnum].sides[sidenum].tmap_num, sizeof(short), 1, LoadFile );
				Segments[segnum].sides[sidenum].tmap_num = read_short_swap(LoadFile);
				// Read short Segments[segnum].sides[sidenum].tmap_num2;
//				cfread( &Segments[segnum].sides[sidenum].tmap_num2, sizeof(short), 1, LoadFile );
				Segments[segnum].sides[sidenum].tmap_num2 = read_short_swap(LoadFile);
				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	{
//					cfread( &temp_short, sizeof(short), 1, LoadFile );
					temp_short = read_short_swap(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].u = ((fix)temp_short) << 5;
//					cfread( &temp_short, sizeof(short), 1, LoadFile );
					temp_short = read_short_swap(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].v = ((fix)temp_short) << 5;
//					cfread( &temp_ushort, sizeof(temp_ushort), 1, LoadFile );
					temp_ushort = read_short_swap(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].l = ((fix)temp_ushort) << 1;
//					cfread( &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1, LoadFile );
				}
			} else {
				Segments[segnum].sides[sidenum].tmap_num = 0;
				Segments[segnum].sides[sidenum].tmap_num2 = 0;
				for (i=0; i<4; i++ )	{
					Segments[segnum].sides[sidenum].uvls[i].u = 0;
					Segments[segnum].sides[sidenum].uvls[i].v = 0;
					Segments[segnum].sides[sidenum].uvls[i].l = 0;
				}
			}
		}
	}

	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	validate_segment_all();			// Fill in side type and normals.

	// Activate fuelcentes
	for (i=0; i< Num_segments; i++ ) {
		fuelcen_activate( &Segments[i], Segments[i].special );
	}

	reset_objects(1);		//one object, the player

	return 0;
}

#ifndef MAC_SHAREWARE
static int load_mine_data_compiled_new(CFILE *LoadFile)
{
	int		i,segnum,sidenum;
	ubyte		version;
	short		temp_short;
	ushort	temp_ushort;
	ubyte		bit_mask;

	//	For compiled levels, textures map to themselves, prevent tmap_override always being gray,
	//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
#ifdef EDITOR
	for (i=0; i<MAX_TEXTURES; i++)
		tmap_xlate_table[i] = i;
#endif

//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	cfread( &version, sizeof(ubyte), 1, LoadFile );						// 1 byte = compiled version
	Assert( version==COMPILED_MINE_VERSION );

	Num_vertices = read_short_swap(LoadFile);
	Assert( Num_vertices <= MAX_VERTICES );

	Num_segments = read_short_swap(LoadFile);
	Assert( Num_segments <= MAX_SEGMENTS );

	for (i = 0; i < Num_vertices; i++) {
		Vertices[i].x = read_fix_swap(LoadFile);
		Vertices[i].y = read_fix_swap(LoadFile);
		Vertices[i].z = read_fix_swap(LoadFile);
	}

	for (segnum=0; segnum<Num_segments; segnum++ )	{
		int	bit;

		#ifdef EDITOR
		Segments[segnum].segnum = segnum;
		Segments[segnum].group = 0;
		#endif

		bit_mask = read_byte(LoadFile);

		for (bit=0; bit<MAX_SIDES_PER_SEGMENT; bit++) {
			if (bit_mask & (1 << bit))
				Segments[segnum].children[bit] = read_short_swap(LoadFile);
			else
				Segments[segnum].children[bit] = -1;
		}

		// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
		for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
			Segments[segnum].verts[i] = read_short_swap(LoadFile);

		Segments[segnum].objects = -1;

		if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
			// Read ubyte	Segments[segnum].special
			Segments[segnum].special = read_byte(LoadFile);
			// Read byte	Segments[segnum].matcen_num
			Segments[segnum].matcen_num = read_byte(LoadFile);
			// Read short	Segments[segnum].value
			Segments[segnum].value = read_short_swap(LoadFile);
		} else {
			Segments[segnum].special = 0;
			Segments[segnum].matcen_num = -1;
			Segments[segnum].value = 0;
		}

		// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
		temp_ushort = read_short_swap(LoadFile);
		Segments[segnum].static_light	= ((fix)temp_ushort) << 4;

		// Read the walls as a 6 byte array
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			Segments[segnum].sides[sidenum].pad = 0;
		}

		bit_mask = read_byte(LoadFile);
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			ubyte byte_wallnum;

			if (bit_mask & (1 << sidenum)) {
				byte_wallnum = read_byte(LoadFile);
				if ( byte_wallnum == 255 )
					Segments[segnum].sides[sidenum].wall_num = -1;
				else
					Segments[segnum].sides[sidenum].wall_num = byte_wallnum;
			} else
					Segments[segnum].sides[sidenum].wall_num = -1;
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{

			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				// Read short Segments[segnum].sides[sidenum].tmap_num;
				temp_ushort = read_short_swap(LoadFile);
				Segments[segnum].sides[sidenum].tmap_num = temp_ushort & 0x7fff;

				if (!(temp_ushort & 0x8000))
					Segments[segnum].sides[sidenum].tmap_num2 = 0;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					Segments[segnum].sides[sidenum].tmap_num2 = read_short_swap(LoadFile);
				}

				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	{
					temp_short = read_short_swap(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].u = ((fix)temp_short) << 5;
					temp_short = read_short_swap(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].v = ((fix)temp_short) << 5;
					temp_ushort = read_short_swap(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].l = ((fix)temp_ushort) << 1;
				}
			} else {
				Segments[segnum].sides[sidenum].tmap_num = 0;
				Segments[segnum].sides[sidenum].tmap_num2 = 0;
				for (i=0; i<4; i++ )	{
					Segments[segnum].sides[sidenum].uvls[i].u = 0;
					Segments[segnum].sides[sidenum].uvls[i].v = 0;
					Segments[segnum].sides[sidenum].uvls[i].l = 0;
				}
			}
		}
	}

	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	validate_segment_all();			// Fill in side type and normals.

	// Activate fuelcentes
	for (i=0; i< Num_segments; i++ ) {
		fuelcen_activate( &Segments[i], Segments[i].special );
	}

	reset_objects(1);		//one object, the player

	return 0;
}

#endif
