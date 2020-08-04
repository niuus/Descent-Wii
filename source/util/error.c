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

#include <ogcsys.h>
#include <asndlib.h>
#include <ogc/machine/processor.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dtypes.h"
#include "error.h"
#include "settings.h"

#include "crash_bin.h"

#include <debug.h>

#define CPU_STACK_TRACE_DEPTH		10

#define CRASH_PIC_WIDTH  400
#define CRASH_PIC_HEIGHT 230

static char tmp_buf[512] = {0};
static void *exception_xfb = (void*)0xC1700000;			//we use a static address above ArenaHi.

extern void udelay(int us);
extern void __reload();
extern void VIDEO_SetFramebuffer(void *);

typedef struct _framerec {
	struct _framerec *up;
	void *lr;
} frame_rec, *frame_rec_t;

static const char *exception_name[NUM_EXCEPTIONS] = {
		"System Reset", "Machine Check", "DSI", "ISI",
		"Interrupt", "Alignment", "Program", "Floating Point",
		"Decrementer", "System Call", "Trace", "Performance",
		"IABR", "Reserved", "Thermal"};

static void _cpu_print_stack(void *pc,void *lr,void *r1)
{
	register u32 i = 0;
	register frame_rec_t l,p = (frame_rec_t)lr;

	l = p;
	p = r1;
	if(!p) __asm__ __volatile__("mr %0,%%r1" : "=r"(p));

	kprintf("\n\tSTACK DUMP:");

	for(i=0;i<CPU_STACK_TRACE_DEPTH-1 && p->up;p=p->up,i++) {
		if(i%4) kprintf(" --> ");
		else {
			if(i>0) kprintf(" -->\n\t");
			else kprintf("\n\t");
		}

		switch(i) {
			case 0:
				if(pc) kprintf("%p",pc);
				break;
			case 1:
				if(!l) l = (frame_rec_t)mfspr(8);
				kprintf("%p",(void*)l);
				break;
			default:
				kprintf("%p",(void*)(p->up->lr));
				break;
		}
	}
}

static void waitForReload(int allow_continue)
{
	u32 level;

	while ( 1 )
	{
		if (SYS_ResetButtonDown())
		{
			if (allow_continue) {
				udelay(500000);
				if (!SYS_ResetButtonDown()) {
					printf("\n\tContinuing (may be unstable)...\n\n\n");
					break;
				}
			}
			kprintf("\n\tExiting...\n\n\n");
			_CPU_ISR_Disable(level);
			__reload();
		}

		VIDEO_SetFramebuffer(exception_xfb);
		VIDEO_Flush();
		udelay(20000);
	}
}

// I use my own exception handler so I can pause the audio DMA engine
// (prevents the same samples repeating over and over while in the exception)
void my_exceptionhandler(frame_context *pCtx)
{
	const unsigned int pitch = 1280;
	int cont = 0;
	ASND_Pause(1);

	VIDEO_SetBlack(TRUE);

	VIDEO_SetFramebuffer(exception_xfb);

	// clear whole console
	CON_Init(exception_xfb,20,20,pitch/2,574,pitch);

	if (pCtx->EXCPT_Number==EX_PRG && pCtx->SRR1&0x00020000) { // it's a trap!
		int i;
		u8 *fb = (u8*)exception_xfb;

		// place stupid picture
		for (i=0; i < CRASH_PIC_HEIGHT; i++)
			// be very careful changing this line, fb is uncached so offset must be guaranteed to be 4-byte aligned
			memcpy(fb+i*pitch+20*2+pitch/2-CRASH_PIC_WIDTH+10*pitch, crash_bin+i*CRASH_PIC_WIDTH*2, CRASH_PIC_WIDTH*2);

		// position console under picture
		fb += (CRASH_PIC_HEIGHT+10)*pitch;
		CON_Init(fb,20,240,pitch/2,200,pitch);
		kprintf("\n\n\n\tTrapped an error:\n\t%s\n", tmp_buf);
		tmp_buf[0] = '\0';
		kprintf("\n\tTap the Reset button to continue or hold it down to exit.");
		cont = 1;
		// if we return, proceed to next instruction
		pCtx->SRR0 += 4;
	}
	else
	{
		kprintf("\n\n\n\tException (%s) occurred!\n", exception_name[pCtx->EXCPT_Number]);

		kprintf("\tGPR00 %08X GPR08 %08X GPR16 %08X GPR24 %08X\n",pCtx->GPR[0], pCtx->GPR[ 8], pCtx->GPR[16], pCtx->GPR[24]);
		kprintf("\tGPR01 %08X GPR09 %08X GPR17 %08X GPR25 %08X\n",pCtx->GPR[1], pCtx->GPR[ 9], pCtx->GPR[17], pCtx->GPR[25]);
		kprintf("\tGPR02 %08X GPR10 %08X GPR18 %08X GPR26 %08X\n",pCtx->GPR[2], pCtx->GPR[10], pCtx->GPR[18], pCtx->GPR[26]);
		kprintf("\tGPR03 %08X GPR11 %08X GPR19 %08X GPR27 %08X\n",pCtx->GPR[3], pCtx->GPR[11], pCtx->GPR[19], pCtx->GPR[27]);
		kprintf("\tGPR04 %08X GPR12 %08X GPR20 %08X GPR28 %08X\n",pCtx->GPR[4], pCtx->GPR[12], pCtx->GPR[20], pCtx->GPR[28]);
		kprintf("\tGPR05 %08X GPR13 %08X GPR21 %08X GPR29 %08X\n",pCtx->GPR[5], pCtx->GPR[13], pCtx->GPR[21], pCtx->GPR[29]);
		kprintf("\tGPR06 %08X GPR14 %08X GPR22 %08X GPR30 %08X\n",pCtx->GPR[6], pCtx->GPR[14], pCtx->GPR[22], pCtx->GPR[30]);
		kprintf("\tGPR07 %08X GPR15 %08X GPR23 %08X GPR31 %08X\n",pCtx->GPR[7], pCtx->GPR[15], pCtx->GPR[23], pCtx->GPR[31]);
		kprintf("\tLR %08X SRR0 %08x SRR1 %08x MSR %08x\n", pCtx->LR, pCtx->SRR0, pCtx->SRR1, pCtx->MSR);
		kprintf("\tDAR %08X DSISR %08X\n", mfspr(19), mfspr(18));

		_cpu_print_stack((void*)pCtx->SRR0,(void*)pCtx->LR,(void*)pCtx->GPR[1]);

		if((pCtx->EXCPT_Number==EX_DSI) || (pCtx->EXCPT_Number==EX_FP)) {
			u32 i;
			u32 *pAdd = (u32*)pCtx->SRR0;
			kprintf("\n\n\tCODE DUMP:\n");
			for (i=0; i<12; i+=4)
				kprintf("\t%p:  %08X %08X %08X %08X\n",
				pAdd+i, pAdd[i], pAdd[i+1], pAdd[i+2], pAdd[i+3]);
		}

		kprintf("\n\tPress the Reset button to exit.");
	}

	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();

	waitForReload(cont);

	ASND_Pause(0);
}

void Error(char *format, ...)
{
	va_list args;

	memset(tmp_buf, 0, sizeof(tmp_buf));
	va_start(args, format);
	vsprintf(tmp_buf, format, args);
	va_end(args);

	// _break();
	__asm__ __volatile__("trap");
}

void Warning(char *format, ...)
{
	va_list args;

	memset(tmp_buf, 0, sizeof(tmp_buf));
	va_start(args, format);
	vsprintf(tmp_buf, format, args);
	va_end(args);
	puts("WARNING: ");
	puts(tmp_buf);

}

void MyAssert(int expr, char *expr_text, char *filename, int linenum)
{
	if (!(expr))
		Error("Assertion failed: %s, file %s, line %d", expr_text, filename, linenum);
}
