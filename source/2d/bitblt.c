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

#include "gr.h"
#include "rle.h"
#include "grdef.h"
#include "error.h"
#include "byteswap.h"

#include <string.h>

ubyte *gr_bitblt_fade_table=NULL;

void gr_linear_movsd_double(ubyte *src, ubyte *dest, uint num_pixels)
{
	int		i;
	uint	*d = (uint *)dest;
	uint	*s = (uint *)src;
	uint work[2];

	num_pixels >>= 2;

	for (i = 0; i < num_pixels; i++) {
		uint x = s[i];

		work[0] = ((x >> 8) & 0x00FFFF00) | (x & 0xFF000000) | ((x >> 16) & 0x000000FF); // 0xABCDEFGH -> 0xABABCDCD
		work[1] = ((x << 8) & 0x00FFFF00) | (x & 0x000000FF) | ((x << 16) & 0xFF000000); // 0xABCDEFGH -> 0xEFEFGHGH

		d[i*2+0] = work[0];
		d[i*2+1] = work[1];
	}
}

void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, int num_pixels )
{
	int i;
	for (i=0; i<num_pixels; i++) {
		if (src[i] != TRANSPARENCY_COLOR)
			dest[i] = src[i];
	}
}

void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, uint num_pixels, ubyte fade_value )
{
	int i;
	ubyte *fade_base;

	fade_base = gr_fade_table + (fade_value * 256);

	for (i=0; i<num_pixels; i++) {
		if (src[i] != TRANSPARENCY_COLOR)
			dest[i] = fade_base[src[i]];
	}

}

void gr_bm_ubitblt_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;

	int i;

	sbits = src->bm_data + 4 + src->bm_h;
	for (i=0; i<sy; i++ )
		sbits += src->bm_data[4+i];

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ ) {
		gr_rle_expand_scanline(dbits, sbits, sx, sx+w-1, 0);
		sbits += src->bm_data[4+i+sy];
		dbits += dest->bm_rowsize;
	}
}

void gr_bitblt_cockpit(grs_bitmap *bm)
{
	gr_bm_ubitblt_rle(bm->bm_w, bm->bm_h, 0, 0, 0, 0, bm, &grd_curcanv->cv_bitmap);
}

void gr_bm_ubitbltm_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;

	int i;

	sbits = src->bm_data + 4 + src->bm_h;
	for (i=0; i<sy; i++ )
		sbits += src->bm_data[4+i];

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ ) {
		gr_rle_expand_scanline_masked(dbits, sbits, sx, sx+w-1);
		sbits += src->bm_data[4+i+sy];
		dbits += dest->bm_rowsize;
	}
}

void gr_ubitmap( int x, int y, grs_bitmap *bm )
{
	register int y1;
	int dest_rowsize;

	unsigned char * dest;
	unsigned char * src;

	if ( bm->bm_flags & BM_FLAG_RLE) {
		gr_bm_ubitblt_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
		return;
	}

	dest_rowsize = grd_curcanv->cv_bitmap.bm_rowsize;
	dest = grd_curcanv->cv_bitmap.bm_data + (dest_rowsize * y) + x;

	src = bm->bm_data;

	for (y1=0; y1 < bm->bm_h; y1++ )    {
		gr_linear_movsd(src, dest, bm->bm_w);
		src += bm->bm_rowsize;
		dest += dest_rowsize;
	}
}

void gr_ubitmapm( int x, int y, grs_bitmap *bm )
{
	register int y1;
	int dest_rowsize;

	unsigned char * dest;
	unsigned char * src;

	if ( bm->bm_flags & BM_FLAG_RLE )	{
		gr_bm_ubitbltm_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
		return;
	}

	dest_rowsize=grd_curcanv->cv_bitmap.bm_rowsize;
	dest = &(grd_curcanv->cv_bitmap.bm_data[ dest_rowsize*y+x ]);

	src = bm->bm_data;

	if (gr_bitblt_fade_table==NULL)	{
		for (y1=0; y1 < bm->bm_h; y1++ )    {
			gr_linear_rep_movsdm( src, dest, bm->bm_w );
			src += bm->bm_rowsize;
			dest+= dest_rowsize;
		}
	} else {
		for (y1=0; y1 < bm->bm_h; y1++ )    {
			gr_linear_rep_movsdm_faded( src, dest, bm->bm_w, gr_bitblt_fade_table[y1+y] );
			src += bm->bm_rowsize;
			dest+= dest_rowsize;
		}
	}
}

extern void BlitLargeAlign(ubyte *draw_buffer, int dstRowBytes, ubyte *dstPtr, int w, int h, int modulus);

void gr_bm_ubitblt_double(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap *src, grs_bitmap *dest)
{
	ubyte * dbits;
	ubyte * sbits;
	int dstep, i;

	sbits = src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	dstep = dest->bm_rowsize;
#if defined(__powerc) || ( defined(__MWERKS__) && defined(__MC68K__) && defined(USE_2D_ASM) )
	BlitLargeAlign(sbits, dstep, dbits, src->bm_w, src->bm_h, src->bm_rowsize);
#else
	for (i=0; i < h; i++ )    {
		gr_linear_movsd_double(sbits, dbits, w);
		dbits += dstep;
		gr_linear_movsd_double(sbits, dbits, w);
		dbits += dstep;
		sbits += src->bm_rowsize;
	}
#endif
}

// From Linear to Linear
void gr_bm_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;
	int dstep;

	int i;

	if ( src->bm_flags & BM_FLAG_RLE )	{
		gr_bm_ubitblt_rle( w, h, dx, dy, sx, sy, src, dest );
		return ;
	}

	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;

	dstep = dest->bm_rowsize;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_linear_movsd(sbits, dbits, w);
		sbits += src->bm_rowsize;
		dbits += dstep;
	}
}

// From Linear to Linear Masked
void gr_bm_ubitbltm(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;

	int i;

	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.

	if (gr_bitblt_fade_table==NULL)	{
		for (i=0; i < h; i++ )    {
			gr_linear_rep_movsdm( sbits, dbits, w );
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	} else {
		for (i=0; i < h; i++ )    {
			gr_linear_rep_movsdm_faded( sbits, dbits, w, gr_bitblt_fade_table[dy+i] );
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	}
}


void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int dx1=dx, dx2=dx+dest->bm_w-1;
	int dy1=dy, dy2=dy+dest->bm_h-1;

	int sx1=sx, sx2=sx+src->bm_w-1;
	int sy1=sy, sy2=sy+src->bm_h-1;

	if ((dx1 >= dest->bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= dest->bm_h ) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx1 += -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy1 += -dy1; dy1 = 0; }
	if ( dx2 >= dest->bm_w )	{ dx2 = dest->bm_w-1; }
	if ( dy2 >= dest->bm_h )	{ dy2 = dest->bm_h-1; }

	if ((sx1 >= src->bm_w ) || (sx2 < 0)) return;
	if ((sy1 >= src->bm_h ) || (sy2 < 0)) return;
	if ( sx1 < 0 ) { dx1 += -sx1; sx1 = 0; }
	if ( sy1 < 0 ) { dy1 += -sy1; sy1 = 0; }
	if ( sx2 >= src->bm_w )	{ sx2 = src->bm_w-1; }
	if ( sy2 >= src->bm_h )	{ sy2 = src->bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	if ( dx2-dx1+1 < w )
		w = dx2-dx1+1;
	if ( dy2-dy1+1 < h )
		h = dy2-dy1+1;
	if ( sx2-sx1+1 < w )
		w = sx2-sx1+1;
	if ( sy2-sy1+1 < h )
		h = sy2-sy1+1;

	gr_bm_ubitblt(w,h, dx1, dy1, sx1, sy1, src, dest );
}


// Clipped bitmap ...

void gr_bitmap( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w )	{ dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h )	{ dy2 = grd_curcanv->cv_bitmap.bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	gr_bm_ubitblt(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
}

void gr_bitmapm( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w )	{ dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h )	{ dy2 = grd_curcanv->cv_bitmap.bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	if ( bm->bm_flags & BM_FLAG_RLE )
		gr_bm_ubitbltm_rle(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
	else
		gr_bm_ubitbltm(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
}

void scale_up_bitmap(grs_bitmap *source_bmp, grs_bitmap *dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0,  fix u1, fix v1);
void scale_bitmap_c(grs_bitmap *source_bmp, grs_bitmap *dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0,  fix u1, fix v1);

void gr_bitmap_scale(grs_bitmap *bm)
{
	grs_bitmap *dest = &grd_curcanv->cv_bitmap;
	// slow but only done from init_cockpit
	scale_bitmap_c(bm, dest, 0, 0, dest->bm_w-1, dest->bm_h-1, 0, 0, F1_0*(bm->bm_w-1), F1_0*(bm->bm_h-1));
}

void gr_bitmapm_scale(int x, int y, int x0, int y0, grs_bitmap *bm)
{
	grs_bitmap *dest = &grd_curcanv->cv_bitmap;
	scale_up_bitmap(bm, dest, x, y, dest->bm_w-1, dest->bm_h-1, F1_0*x0, F1_0*y0, F1_0*(bm->bm_w-1), F1_0*(bm->bm_h-1));
}
