#ifndef _TYPES_H
#define _TYPES_H

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#endif

//define a byte
typedef signed char byte;

//define unsigned types;
typedef unsigned char ubyte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

//define a boolean
#ifndef bool
typedef ubyte bool;
#endif

typedef struct {
	int h;
	int v;
} Point;

#ifndef NULL
#define NULL 0
#endif

#endif

