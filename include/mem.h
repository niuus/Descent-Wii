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

extern int show_mem_info;

void * mem_display_blocks();
extern void * mem_malloc( unsigned int size, char * var, char * file, int line, int fill_zero );
extern void * mem_malloc_align( unsigned int size, char * var, char * file, int line, int align, int fill_zero );
extern void mem_free( void * buffer );

#define mymalloc(size)    mem_malloc((size),"Unknown", __FILE__,__LINE__, 0 )
//#define mymalloc_align(size, align) mem_malloc_align((size), "Unknown", __FILE__, __LINE__, align, 0)
#define mymalloc_align(size, align) mem_malloc((size), "Unknown", __FILE__, __LINE__, 0)
#define mycalloc(n,size)  mem_malloc((n*size),"Unknown", __FILE__,__LINE__, 1 )
#define myfree(ptr)       do{ mem_free(ptr); ptr=NULL; } while(0)

#define malloc(size)    mem_malloc((size),"Unknown", __FILE__,__LINE__, 0 )
#define calloc(n,size)  mem_malloc((n*size),"Unknown", __FILE__,__LINE__, 1 )
#define free(ptr)       do{ mem_free(ptr); ptr=NULL; } while(0)

#define MALLOC( var, type, count )   (var=(type *)mem_malloc((count)*sizeof(type),#var, __FILE__,__LINE__,0 ))

// Checks to see if any blocks are overwritten
void mem_validate_heap();

