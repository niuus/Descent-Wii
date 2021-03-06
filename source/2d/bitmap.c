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

#include <stdlib.h>
#include <stdio.h>

#include "mem.h"
#include "gr.h"
#include "grdef.h"

grs_bitmap *gr_create_bitmap(int w, int h )
{
    grs_bitmap *new;

    new = (grs_bitmap *)mymalloc( sizeof(grs_bitmap) );
	new->bm_x = 0;
	new->bm_y = 0;
    new->bm_w = w;
    new->bm_h = h;
	new->bm_flags = 0;
	new->bm_type = 0;
	new->bm_rowsize = w;
	new->bm_selector = 0;

    new->bm_data = (unsigned char *)mymalloc_align( w*h, 8 );

    return new;
}

grs_bitmap *gr_create_bitmap_raw(int w, int h, unsigned char * raw_data )
{
	grs_bitmap *new;

	new = (grs_bitmap *)mymalloc( sizeof(grs_bitmap) );
	new->bm_x = 0;
	new->bm_y = 0;
	new->bm_w = w;
	new->bm_h = h;
	new->bm_flags = 0;
	new->bm_type = 0;
	new->bm_rowsize = w;
	new->bm_data = raw_data;
	new->bm_selector = 0;

	return new;
}

void gr_init_bitmap( grs_bitmap *bm, int mode, int x, int y, int w, int h, int bytesperline, unsigned char * data )
{
	bm->bm_x = x;
	bm->bm_y = y;
	bm->bm_w = w;
	bm->bm_h = h;
	bm->bm_flags = 0;
	bm->bm_type = mode;
	bm->bm_rowsize = bytesperline;
	bm->bm_data = data;
	bm->bm_selector = 0;
}


grs_bitmap *gr_create_sub_bitmap(grs_bitmap *bm, int x, int y, int w, int h )
{
    grs_bitmap *new;

    new = (grs_bitmap *)mymalloc( sizeof(grs_bitmap) );
	new->bm_x = x+bm->bm_x;
	new->bm_y = y+bm->bm_y;
	new->bm_w = w;
    new->bm_h = h;
	new->bm_flags = bm->bm_flags;
	new->bm_type = bm->bm_type;
	new->bm_rowsize = bm->bm_rowsize;
	new->bm_data = bm->bm_data+(unsigned int)((y*bm->bm_rowsize)+x);
	new->bm_selector = 0;

	return new;
}


void gr_free_bitmap(grs_bitmap *bm )
{
	if (bm->bm_data!=NULL)
    	myfree(bm->bm_data);
	bm->bm_data = NULL;
	if (bm!=NULL)
    	myfree(bm);
}

void gr_free_sub_bitmap(grs_bitmap *bm )
{
	if (bm!=NULL)
    	myfree(bm);
}

void decode_data_asm(ubyte *data, int num_pixels, ubyte *colormap, int *count)
{
	int i;

	for (i = 0; i < num_pixels; i++) {
		count[*data]++;
		*data = colormap[*data];
		data++;
	}
}

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq )
{
	int i, r, g, b;

	for (i=0; i<256; i++ )	{
		r = *palette++;
		g = *palette++;
		b = *palette++;
 		*colormap++ = gr_find_closest_color( r, g, b );
		*freq++ = 0;
	}
}

void gr_remap_bitmap( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256], i, n;

	// This should be build_colormap_asm, but we're not using invert table, so...
	build_colormap_good( palette, colormap, freq );

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = TRANSPARENCY_COLOR;

	n = bmp->bm_w * bmp->bm_h;
	for (i = 0; i < n; i++)
		bmp->bm_data[i] = colormap[bmp->bm_data[i]];
	decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );

	if ( (transparent_color>=0) && (transparent_color<=255) && (freq[transparent_color]>0) )
		bmp->bm_flags |= BM_FLAG_TRANSPARENT;

	if ( (super_transparent_color>=0) && (super_transparent_color<=255) && (freq[super_transparent_color]>0) )
		bmp->bm_flags |= BM_FLAG_SUPER_TRANSPARENT;
}

void gr_remap_bitmap_good( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256];

	build_colormap_good( palette, colormap, freq );

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = TRANSPARENCY_COLOR;

	decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );

	if ( (transparent_color>=0) && (transparent_color<=255) && (freq[transparent_color]>0) )
		bmp->bm_flags |= BM_FLAG_TRANSPARENT;

	if ( (super_transparent_color>=0) && (super_transparent_color<=255) && (freq[super_transparent_color]>0) )
		bmp->bm_flags |= BM_FLAG_SUPER_TRANSPARENT;
}


int gr_bitmap_assign_selector( grs_bitmap * bmp )
{
	bmp->bm_selector = 0;
	return 0;
}

void gr_bitmap_check_transparency( grs_bitmap * bmp )
{
	int x, y;
	ubyte * data;

	data = bmp->bm_data;

	for (y=0; y<bmp->bm_h; y++ )	{
		for (x=0; x<bmp->bm_w; x++ )	{
			if (*data++ == TRANSPARENCY_COLOR )	{
				bmp->bm_flags = BM_FLAG_TRANSPARENT;
				return;
			}
		}
		data += bmp->bm_rowsize - bmp->bm_w;
	}

	bmp->bm_flags = 0;

}
