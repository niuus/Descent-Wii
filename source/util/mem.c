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
#include <stdarg.h>
#include <string.h>

#include "dtypes.h"
#include "mono.h"
#include "error.h"

static int Initialized = 0;

static int pointers[1000];
static int sizes[1000];

void mem_display_blocks();


#define CHECKSIZE 16

int show_mem_info = 0;
int Free_heap_space = 0;

void mem_init()
{
	int i;

	for (i = 0; i < 1000; i++) {
		pointers[i] = -1;
		sizes[i] = -1;
	}

	Initialized = 1;

	atexit(mem_display_blocks);
}

#define STACK_WARN_LIMIT	(3 * 1024)
#define HEAP_WARN_LIMIT		(300 * 1024)
void check_mem_conditions()
{
	//int total, contig;

#if 0
	if (StackSpace() < STACK_WARN_LIMIT)
		Warning("Stack nearning limit!!!");

	if (FreeMem() < HEAP_WARN_LIMIT) {
		PurgeSpace(&total, &contig);
//		Warning("Free memory reaching limit\n.Purging would give %d total %d contiguous.\nLet me know Descent minimum and suggested size!!!", total, contig);
	}
#endif
}



void * mem_malloc( unsigned int size, char * var, char * filename, int line, int fill_zero )
{
	void *ptr;

	if (Initialized==0)
		mem_init();

	if (size==0)	{
		Warning("Attempt to malloc 0 bytes.\nVar %s, file %s, line %d.\n", var, filename, line);
		Error( "MEM_MALLOC_ZERO" );
	}

	ptr = malloc(size+CHECKSIZE);

	if (ptr==NULL)	{
		Warning("Malloc returned NULL\nVar %s, file %s, line %d.\n", var, filename, line );
		Error( "MEM_OUT_OF_MEMORY" );
	}
	memset(ptr, 0, size+CHECKSIZE);
//	Free_heap_space = FreeMem();

	check_mem_conditions();
	return ptr;
}

void mem_free( void * buffer )
{
	//int i, j;
	void *tmp;

	if (Initialized==0)
		mem_init();

	if (buffer==NULL)
		Warning( "MEM: Freeing the NULL pointer!" );

	tmp = buffer;

	free(tmp);
//	Free_heap_space = FreeMem();
	check_mem_conditions();
	buffer = NULL;
}

void mem_display_blocks()
{
}

void mem_validate_heap()
{
}

void mem_print_all()
{
}


