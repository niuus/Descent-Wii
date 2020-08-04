#include <ppc-asm.h>

.equ fpTemp,		f0
.equ rSrcPtr,		r3
.equ rDblDStrd,		r4
.equ rDst1Ptr,		r5
.equ rWidth,		r6
.equ rHeight,		r7
.equ rModulus,		r8
.equ rSStrd,		r9
.equ rLDblPtr,		r10
.equ rDst2Ptr,		r11

# 
#  void BlitLargeAlign(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus)
#                      r3                  r4               r5            r6       r7       r8
# 

FUNC_START(BlitLargeAlign)
	addi	rDst1Ptr,rDst1Ptr,-8		# subtract 8 from dst

	la	rLDblPtr,-8(sp)			# calculate copy of local 8-byte variable (red-zone)
	sub	rSStrd,rModulus,rWidth		# rSStrd = modulus - w
	add	rDst2Ptr,rDst1Ptr,r4		# dst2 = dstRowBytes + dst1
	sub	r4,r4,rWidth			# r4 = dstRowBytes - w
	addi	rHeight,rHeight,-1		# subtract 1 from height count
	srawi	rWidth,rWidth,2			# rWidth = w >> 2
	addi	rSrcPtr,rSrcPtr,-4		# subtract 4 from src
	addi	rWidth,rWidth,-1		# subtract 1 from width count
	add	rDblDStrd,r4,r4			# rDblDStrd = 2 * r4

BlitLargeAlignY:				# y count is in rHeight
	lwzu	r10,4(rSrcPtr)			# load a long into r10
	mr	r0,r10  			# put a copy in r0
	mr	r11,r10
	inslwi	r0,r10,16,8
	insrwi	r11,r10,16,8
	rlwimi	r0,r10,16,24,31
	stw	r0,0(rLDblPtr)
	rlwimi	r11,r10,16,0,7
	stw	r11,4(rLDblPtr)
	mtctr	rWidth				# copy x count into the counter
	lfd	fpTemp,0(rLDblPtr)

BlitLargeAlignX:
	lwzu	r10,4(rSrcPtr)			# load a long into r10
	stfdu	fpTemp,8(rDst1Ptr)
	mr	r0,r10  			# put a copy in r0
	mr	r11,r10
	inslwi	r0,r10,16,8
	insrwi	r11,r10,16,8
	rlwimi	r0,r10,16,24,31
	stw	r0,0(rLDblPtr)
	rlwimi	r11,r10,16,0,7
	stw	r11,4(rLDblPtr)
	stfdu	fpTemp,8(rDst2Ptr)
	lfd	fpTemp,0(rLDblPtr)
	bdnz	BlitLargeAlignX			# loop over all x

	stfdu	fpTemp,8(rDst1Ptr)
	addic.	rHeight,rHeight,-1		# decrement the counter
	add	rSrcPtr,rSrcPtr,rSStrd		# src += sstride
	add	rDst1Ptr,rDst1Ptr,rDblDStrd	# dst1 += dstride
	stfdu	fpTemp,8(rDst2Ptr)
	add	rDst2Ptr,rDst2Ptr,rDblDStrd	# dst2 += dstride
	bne	BlitLargeAlignY      		# loop for all y

	blr					# return to caller


