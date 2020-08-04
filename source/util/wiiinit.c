#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

#include "error.h"
#include "settings.h"
#include "fileutil.h"

#include <ogcsys.h>
#include <debug.h>
#include <fat.h>
#include <sys/iosupport.h>
#include <sdcard/wiisd_io.h>
#include <sdcard/gcsd.h>
#include <ogc/machine/processor.h>
#include <zlib.h>

#define BUF_SIZE 0x10000

void descent_exceptionhandler();
void __exception_sethandler(u32 nExcept, void (*pHndl)(frame_context*));

static const struct hash_t {
	const char *name;
	const uLong hash;
} hashes[] = {
	{
		"descent.hog",
		0x3B0B1041
	},
	{
		"descent.pig",
		0x0AB80428
	}
};

u8 use_alt_textures = 0;

void do_appl_quit( void )
{
	exit(0);
}

// this is to make sure the libfat cache gets flushed to disk
static void Unmount(void)
{
#ifdef __wii__
	fatUnmount("sd:");
	fatUnmount("usb:");
#else
	fatUnmount("gcsd:");
#endif
}

static void CDToDescentDir()
{
#ifdef __wii__
	if (__system_argv->argvMagic!=ARGV_MAGIC || __system_argv->argc < 1 || strncmp(__system_argv->argv[0], "usb:/", 5)) {
		if (fatMount("sd", &__io_wiisd, 0, 32, 16)) {
			printf("Mounted SD\n");
			if (chdir("sd:/apps/descent")==0)
				return;
			printf("Failed to CD to descent directory on SD\n");
			fatUnmount("sd:");
		}
		else
			printf("Failed to mount SD\n");
	}

	if (fatMount("usb", &__io_usbstorage, 0, 32, 16)) {
		printf("Mounted USB\n");
		if (chdir("usb:/apps/descent")==0)
			return;
		printf("Failed to CD to descent directory on USB\n");
		fatUnmount("usb:");
	}
	else
		printf("Failed to mount USB\n");
#else
	if (fatMount("gcsd", &__io_gcsda, 0, 8, 16)) {
		if (chdir("gcsd:/descent")==0)
			return;
		fatUnmount("gcsd:");
	}
#endif

	atexit(Unmount);
}

static vu32 idle_exit = 0;
static lwp_t idle_thread;

static void pwm_finish(void)
{
	void *x;
	idle_exit = 1;
	LWP_JoinThread(idle_thread, &x);
}

static void* pwm_idle(void *arg)
{
	u32 level, hid0;

	atexit(pwm_finish);

	_CPU_ISR_Disable(level);
	// enable doze mode and DPM
	hid0 = mfspr(HID0);
	mtspr(HID0, (hid0&~0x00E00000)|0x00900000);
	_CPU_ISR_Restore(level);

	while(!idle_exit)
	{
		u32 msr = mfmsr();
		// activate doze mode
		// CPU will sleep until the next interrupt, then wake up and do stuff until this idle thread runs again
		msr |= 0x40000; // MSR[POW] = 1
		_sync();
		mtmsr(msr);
		asm volatile("isync");
	}

	return NULL;
}

int wii_init()
{
#ifndef NDEBUG
	//DEBUG_Init(GDBSTUB_DEVICE_USB, 1);
	CON_EnableGecko(1, 1);
	printf("Wii Debug Console Connected\n");
#endif

	// low priority but above lwp's idle thread
	LWP_CreateThread(&idle_thread, pwm_idle, NULL, NULL, 1024, 1);

	VIDEO_Init();
	GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);
	VIDEO_Configure(rmode);

	// use our own exception handler for everything because the default handlers
	// don't stop GX and AX, which I find very annoying.
	__exception_sethandler(EX_SYS_RESET,descent_exceptionhandler);
	__exception_sethandler(EX_MACH_CHECK,descent_exceptionhandler);
	__exception_sethandler(EX_DSI,descent_exceptionhandler);
	__exception_sethandler(EX_ISI,descent_exceptionhandler);
	__exception_sethandler(EX_ALIGN,descent_exceptionhandler);
	__exception_sethandler(EX_PRG,descent_exceptionhandler);
	__exception_sethandler(EX_TRACE,descent_exceptionhandler);
	__exception_sethandler(EX_PERF,descent_exceptionhandler);
	__exception_sethandler(EX_IABR,descent_exceptionhandler);
	__exception_sethandler(EX_RESV,descent_exceptionhandler);
	__exception_sethandler(EX_THERM,descent_exceptionhandler);

	CDToDescentDir();

#ifdef NDEBUG
	// make sure the resource files are the correct versions
	void *buf = memalign(32, BUF_SIZE);
	if (buf==NULL)
		return 2;

	int i;
	FILE *fp;

	for (i=0; i < sizeof(hashes)/sizeof(hashes[0]); i++) {
		size_t readed;
		uLong crc = crc32(0L, Z_NULL, 0);
		fp = fopen(hashes[i].name, "rb");
		if (fp==NULL) {
			free(buf);
			Error("Couldn't open file %s\n", hashes[i].name);
			return 1;
		}

		while ((readed=fread(buf, 1, BUF_SIZE, fp))>0)
			crc = crc32(crc, buf, readed);

		fclose(fp);
		if (crc != hashes[i].hash) {
			free(buf);
			Error("CRC Mismatch for %s: %08X should be %08X", hashes[i].name, crc, hashes[i].hash);
			return 1;
		}
	}
	free(buf);
#endif

	return 0;
}
