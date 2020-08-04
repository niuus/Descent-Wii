#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "dtypes.h"

#include <gccore.h>
#include <ogcsys.h>
#include <ogc/machine/processor.h>

#include "gr.h"
#include "wiisys.h"
#include "input.h"

// mouse cursor
vu8 cursor_hidden = 1;
#include "d_textures.h"
#include "d_textures_tpl.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define D_SCREEN_WIDTH (SCREEN_WIDTH/sizeof(double))
#define DEFAULT_FIFO_SIZE (128*1024)

// this looks pretty good to me
#define CURSOR_WIDTH 7
#define CURSOR_HEIGHT 10

static sem_t retrace_sema = LWP_SEM_NULL;

static int win_x0, win_y0, win_x1, win_y1;

static u16 wii_palette_a[256] ATTRIBUTE_ALIGN(32);
static u16 wii_palette_b[256] ATTRIBUTE_ALIGN(32);
static double wii_canvas[SCREEN_HEIGHT][D_SCREEN_WIDTH] ATTRIBUTE_ALIGN(32);
static double fb_texture[SCREEN_HEIGHT][D_SCREEN_WIDTH] ATTRIBUTE_ALIGN(32);

const int MonitorRowBytes = SCREEN_WIDTH;   // stride for screen data
const ubyte *MonitorData = (ubyte*)wii_canvas;      // pointer to screen data for monitor;

static unsigned int *xfb[2];
static volatile int whichfb;
static GXRModeObj *rmode;
static unsigned char *gp_fifo;
static GXTlutObj gx_palette_a, gx_palette_b;
static GXTexObj gx_texture_a, gx_texture_b;

static const u8 TexIndex[] ATTRIBUTE_ALIGN(32) = {
	0, 0,
	1, 0,
	1, 1,
	0, 1
};

void hide_cursor()
{
	cursor_hidden = 1;
}

void show_cursor()
{
	cursor_hidden = 0;
}

void set_win_size(int x, int y, int w, int h)
{
	Mtx TexScale;

	win_x0 = x;
	win_y0 = y;
	win_x1 = x+w;
	win_y1 = y+h;
	DCZeroRange(wii_canvas+y*D_SCREEN_WIDTH, SCREEN_WIDTH*h);

	// setup texture co-ord gen matrix to point to new region
	guMtxIdentity(TexScale);
	guMtxScale(TexScale, (float)w/SCREEN_WIDTH, (float)h/SCREEN_HEIGHT, 1.0);
	guMtxTransApply(TexScale, TexScale, (float)x/SCREEN_WIDTH, (float)y/SCREEN_HEIGHT, 0);
	GX_LoadTexMtxImm(TexScale, GX_TEXMTX0, GX_MTX2x4);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
}

void SetEntries(int first, int last, color_record* pal)
{
	while (first <= last) {
		wii_palette_a[pal[first].color_num] = (pal[first].r<<0)|(pal[first].g<<8);
		wii_palette_b[pal[first].color_num] = (pal[first].b<<0);
		first++;
	}
	DCStoreRangeNoSync(wii_palette_a, sizeof(wii_palette_a));
	DCStoreRange(      wii_palette_b, sizeof(wii_palette_b));
	GX_LoadTlut(&gx_palette_a, GX_TLUT0);
	GX_LoadTlut(&gx_palette_b, GX_TLUT1);
	// not strictly necessary since our palette never moves/changes size
	GX_LoadTexObj(&gx_texture_a, GX_TEXMAP0);
	GX_LoadTexObj(&gx_texture_b, GX_TEXMAP2);
}

void Index2Color(int index, color_record* color)
{
	color->color_num = index;
	color->r = (wii_palette_a[index]>>0 )&0xFF;
	color->g = (wii_palette_a[index]>>8 )&0xFF;
	color->b = (wii_palette_b[index]>>0 )&0xFF;
}

static void retrace_cb(u32 cnt)
{
	VIDEO_SetPreRetraceCallback(NULL);
	if (retrace_sema != LWP_SEM_NULL)
		LWP_SemPost(retrace_sema);
}

static void gx_done(void) {
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	VIDEO_SetPreRetraceCallback(retrace_cb);
	whichfb ^= 1;
}

#define DISP_LIST_SIZE 16384
static u32 *DispList;

int wiiwin_init()
{
	Mtx MouseView;
	f32 yscale;
	u32 xfbHeight;
	Mtx44 m;
	TPLFile mouse_tpl;
	GXTexObj mouse_tex;
	GXTexRegion *region_a, *region_b;
	GXTexRegionCallback regionCB;
	u32 disp_list_used;

	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);

	if (rmode == &TVPal528IntDf)
		rmode = &TVPal574IntDfScale;

#ifdef __wii__
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
	{
		rmode->viWidth = VI_MAX_WIDTH_PAL-32;
		rmode->viXOrigin = 16;
		GX_AdjustForOverscan(rmode, rmode, 0, 16);
	}
	else
#endif
		GX_AdjustForOverscan(rmode, rmode, 0, 32);

	VIDEO_Configure(rmode);

	whichfb = 0;
	xfb[0] = (u32*)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	xfb[1] = (u32*)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_ClearFrameBuffer(rmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer(rmode, xfb[1], COLOR_BLACK);
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	VIDEO_SetBlack(FALSE);

	// start GX
	gp_fifo = (unsigned char*)memalign(32, DEFAULT_FIFO_SIZE);
	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

	yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	// have to redo all this after changing yscale
	GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);

	// vertices always use the same co-ords, only the translations change
	GX_SetArray(GX_VA_POS, (void*)TexIndex, 2*sizeof(TexIndex[0]));
	GX_SetArray(GX_VA_TEX0, (void*)TexIndex, 2*sizeof(TexIndex[0]));
	GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc(GX_VA_TEX0, GX_INDEX8);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
	// GX_VTXFMT1 used for mouse cursor (direct pos co-ords)
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_U16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);

	// disable depth buffer
	GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);
	// disable alpha buffer
	GX_SetAlphaUpdate(GX_FALSE);

	// 2 stages used to achieve 24-bit palette texturing
	GX_SetNumTevStages(2);
	GX_SetTevColor(GX_TEVREG0, (GXColor){0xFF, 0xFF, 0x00, 0x00});

	// the matrix used by GX_TEXCOORD0 is configured in set_win_size
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP2, GX_COLORNULL);
	GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);

	GX_SetTevSwapModeTable(GX_TEV_SWAP1, GX_CH_RED,   GX_CH_ALPHA, GX_CH_BLUE, GX_CH_ALPHA);
	GX_SetTevSwapModeTable(GX_TEV_SWAP2, GX_CH_ALPHA, GX_CH_ALPHA, GX_CH_BLUE, GX_CH_ALPHA);

	// stage 0: blue from texture intensity, alpha (0) for other channels
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP2);

	// stage 1: add red from texture intensity and green from texture alpha
	GX_SetTevSwapMode(GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP1);
	GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_CPREV);
	// add to previous stage
	GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

	// Position matrix for main display
	guMtxScale(MouseView, SCREEN_WIDTH, SCREEN_HEIGHT, 1.0);
	GX_LoadPosMtxImm(MouseView, GX_PNMTX0);

	// Position matrix for mouse cursor (scale 320x200 to viewport)
	guMtxTrans(MouseView, 0, 0, -50.0F);
	guMtxScaleApply(MouseView, MouseView, SCREEN_WIDTH/320.0, SCREEN_HEIGHT/200.0, 0);
	GX_LoadPosMtxImm(MouseView, GX_PNMTX1);
	GX_SetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guOrtho(m,0,SCREEN_HEIGHT-1,0,SCREEN_WIDTH-1,0,300);
	GX_LoadProjectionMtx(m, GX_ORTHOGRAPHIC);

	// load mouse cursor
	TPL_OpenTPLFromMemory(&mouse_tpl, (void*)d_textures_tpl, d_textures_tpl_end-d_textures_tpl);
	TPL_GetTexture(&mouse_tpl, gfx_cursor, &mouse_tex);
	GX_LoadTexObj(&mouse_tex, GX_TEXMAP1);
	TPL_CloseTPLFile(&mouse_tpl);

	// init palette and main canvas
	memset(wii_palette_a, 0, sizeof(wii_palette_a));
	memset(wii_palette_b, 0, sizeof(wii_palette_b));
	GX_InitTexObjCI(&gx_texture_a, fb_texture, SCREEN_WIDTH, SCREEN_HEIGHT, GX_TF_CI8, GX_CLAMP, GX_CLAMP, 0, GX_TLUT0);
	GX_InitTexObjCI(&gx_texture_b, fb_texture, SCREEN_WIDTH, SCREEN_HEIGHT, GX_TF_CI8, GX_CLAMP, GX_CLAMP, 0, GX_TLUT1);
	GX_InitTlutObj(&gx_palette_a, wii_palette_a, GX_TL_IA8, 256);
	GX_InitTlutObj(&gx_palette_b, wii_palette_b, GX_TL_IA8, 256);

	DispList = (u32*)memalign(32, DISP_LIST_SIZE);
	DCInvalidateRange(DispList, DISP_LIST_SIZE);

	/* Warning: the following will probably only work on the wii, not GC
	 * because libogc for wii uses static texture regions
	 */
#ifdef __wii__
	// Get the default Texture Region Callback,
	regionCB = GX_SetTexRegionCallback(NULL);
	// find the TexRegion that will be used for our TEXMAPs
	region_a = regionCB(&gx_texture_a, GX_TEXMAP0);
	region_b = regionCB(&gx_texture_b, GX_TEXMAP2);
	// restore the default Texture Region Cllback
	GX_SetTexRegionCallback(regionCB);

	// create display list for square texture render
	GX_BeginDispList(DispList, DISP_LIST_SIZE);
		GX_InvalidateTexRegion(region_a);
		GX_InvalidateTexRegion(region_b);
#else
	GX_BeginDispList(DispList, DISP_LIST_SIZE);
		GX_InvalidateTexAll();
#endif
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			GX_Position1x8(0);
			GX_TexCoord1x8(0);
			GX_Position1x8(1);
			GX_TexCoord1x8(1);
			GX_Position1x8(2);
			GX_TexCoord1x8(2);
			GX_Position1x8(3);
			GX_TexCoord1x8(3);
		GX_End();
	disp_list_used = GX_EndDispList();

	u32 *new_list = (u32*)memalign(32, disp_list_used+32);
	memcpy(new_list+8, DispList, disp_list_used);
	DCFlushRangeNoSync(new_list+8, disp_list_used);
	free(DispList);
	DispList = new_list;
	DispList[0] = disp_list_used;

	LWP_SemInit(&retrace_sema, 1, 1);
	GX_SetDrawDoneCallback(gx_done);

	return 0;
}

void bitblt_to_screen()
{
	int x, y;

	// blit linear data to CI8 texture
	for (y=win_y0; y < win_y1; y+=4) {
		for (x=(win_x0/sizeof(double)); x < (win_x1/sizeof(double)); x+=4) {
			double *out = fb_texture[y]+x*4;

			// read in 32 bytes at a time to use cache
			// 8 doubles at a time = only volatile FPRs will be used
			register double a = wii_canvas[y+0][x+0];
			register double b = wii_canvas[y+0][x+1];
			register double c = wii_canvas[y+0][x+2];
			register double d = wii_canvas[y+0][x+3];
			register double e = wii_canvas[y+1][x+0];
			register double f = wii_canvas[y+1][x+1];
			register double g = wii_canvas[y+1][x+2];
			register double h = wii_canvas[y+1][x+3];

			out[0]  = a;
			out[1]  = e;
			out[4]  = b;
			out[5]  = f;
			out[8]  = c;
			out[9]  = g;
			out[12] = d;
			out[13] = h;

			a = wii_canvas[y+2][x+0];
			b = wii_canvas[y+2][x+1];
			c = wii_canvas[y+2][x+2];
			d = wii_canvas[y+2][x+3];
			e = wii_canvas[y+3][x+0];
			f = wii_canvas[y+3][x+1];
			g = wii_canvas[y+3][x+2];
			h = wii_canvas[y+3][x+3];

			out[2]  = a;
			out[3]  = e;
			out[6]  = b;
			out[7]  = f;
			out[10] = c;
			out[11] = g;
			out[14] = d;
			out[15] = h;

			DCFlushRangeNoSync(out, sizeof(double)*16);
		}
	}
	ppcsync();

	GX_CallDispList(DispList+8, DispList[0]);

#ifdef __wii__
	if (!cursor_hidden) {
		// enable EFB blending for transparent parts of cursor
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
		// switch stage 0 to draw mouse cursor
		GX_SetNumTexGens(2);
		GX_SetNumTevStages(1);
		// load mouse cursor texture
		GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLORNULL);
		// don't use static indexed vertex positions
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
		// scale cursor position to fit 640x480
		GX_SetCurrentMtx(GX_PNMTX1);
		mouse_get_pos(&x, &y);
		GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
				GX_Position2u16(x,              y);
				GX_TexCoord1x8(0);
				GX_Position2u16(x+CURSOR_WIDTH, y);
				GX_TexCoord1x8(1);
				GX_Position2u16(x+CURSOR_WIDTH, y+CURSOR_HEIGHT);
				GX_TexCoord1x8(2);
				GX_Position2u16(x,              y+CURSOR_HEIGHT);
				GX_TexCoord1x8(3);
		GX_End();

		// restore basic attribs
		GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
		GX_SetCurrentMtx(GX_PNMTX0);
		// restore stage 0
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP2, GX_COLORNULL);
		GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP2);
		GX_SetNumTevStages(2);
		GX_SetNumTexGens(1);
		// disable EFB blending
		GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	}
#endif

	LWP_SemWait(retrace_sema);
	GX_CopyDisp(xfb[whichfb], GX_FALSE);
	GX_SetDrawDone();
}
