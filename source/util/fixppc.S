#include <ppc-asm.h>

FUNC_START(fixmul)
	mullw	r5,r3,r4        # multiply low word
	mulhw	r6,r3,r4        # multiply high word (signed)
	srwi	r3,r5,16	# mask off low portion of result
	insrwi  r3,r6,16, 0	# insert high portion of result
	blr                     # return to caller
	
FUNC_START(fixmulaccum)
	lwz     r6,0(r3)
	lwz     r7,4(r3)
	mullw   r8,r4,r5
	mulhw   r9,r4,r5
	addc    r6,r6,r8
	adde    r7,r7,r9
	stw     r6,0(r3)
	stw     r7,4(r3)
	blr

 # fixmuldiv:
 # r3 = multiplier
 # r4 = multiplicand
 # r5 = divisor
 
FUNC_START(fixmuldiv)
	mullw 	r6,r3,r4	# multiply low word
	mulhw 	r4,r3,r4	# multiply high word
	mr 	r3,r6		# pass 64-bit result to divide function

 # fixdivquadlong:
 # r3 = dividend.lo
 # r4 = dividend.hi
 # r5 = divisor

FUNC_START(fixdivquadlong)
	srawi.	r0,r5,31	# get sign of denominator (and check for zero)
	cmpwi	cr7,r4,0	# get sign of numerator
	crxor	31,0,28		# calculate sign of result, put it into cr7 bit 4(SO)
	
	xor	r5,r0,r5	# NOT divisor if originally negative
	subf.	r5,r0,r5	# r5 = abs(divisor)

	beq-	FixDivByZero	# handle divide by zero
	bge	cr7,FixDiv64Common	
				# skip if numerator is positive
	subfic	r3,r3,0		# negate numerator, carry if necessary
	subfze	r4,r4

FixDiv64Common:
	cmplw	r4,r5		# check for overflow

	cntlzw	r6,r5		# normalize 64-bit numerator and denominator
	xor	r4,r4,r3
	slw	r3,r3,r6
	rotlw	r4,r4,r6
	slw	r5,r5,r6
	xor	r4,r4,r3
	
	bge-	FixDivOverflow	# branch if overflow

	srwi	r6,r5,16		
	divwu	r7,r4,r6	# perform 32-bit by 16-bit division
	mullw	r6,r7,r6
	subf	r4,r6,r4	# calculate remainder
	clrlwi	r6,r5,16
	slwi	r4,r4,16
	mullw	r6,r7,r6
	inslwi	r4,r3,16,16
	subfc	r4,r6,r4
	slwi	r3,r3,16
	subfe.	r6,r6,r6
	add	r3,r3,r7
	bge+	2f
1:
	addc	r4,r4,r5	# estimate needs correction
	addze.	r6,r6
	subi	r3,r3,1
	blt-	1b
2:
	srwi	r6,r5,16		
	divwu	r7,r4,r6	# perform 32-bit by 16-bit division
	mullw	r6,r7,r6
	subf	r4,r6,r4	# calculate remainder
	clrlwi	r6,r5,16
	slwi	r4,r4,16
	mullw	r6,r7,r6
	inslwi	r4,r3,16,16
	subfc	r4,r6,r4
	slwi	r3,r3,16
	subfe.	r6,r6,r6
	add	r3,r3,r7
	bge+	2f
1:
	addc	r4,r4,r5	# estimate needs correction
	addze.	r6,r6
	subi	r3,r3,1
	blt-	1b
2:
	cmpwi	r3,0
	bso	cr7,FixDiv64QuotientNeg	
				# see if we need to negate answer
	blt-	FixDivOverflow	# handle overflow case
	blr			# return to caller

FixDiv64QuotientNeg:
	neg.	r3,r3		# negate answer
	blelr

FixDivOverflow:
	cror	28,31,31	# move cr7(so) to cr7(lt)

FixDivByZero:
	lis	r3,0x8000	# load max negative number
	bltlr	cr7		# return if numerator (or result if checking overflow) was negative
	subi	r3,r3,1		# calc max positive number
	blr

FUNC_START(fixdiv)
	stwu	sp,-0x18(sp)
	lis	r6,0x4430	# fix2double
	xoris	r3,r3,0x8000
	lis	r7,0x4330	# int2double
	xoris	r4,r4,0x8000
	stw	r6,8(sp)
	stw	r3,12(sp)
	stw	r7,16(sp)
	stw	r4,20(sp)
	addi	r6,sp,8
	lfs	f2,fix2double_c@sda21(toc)
	lfs	f3,int2double_c@sda21(toc)
	lfd	f0,8(sp)
	lfd	f1,16(sp)
	fsub	f0,f0,f2	# convert fix to double
	fsub	f1,f1,f3	# convert int to double
	fdiv	f1,f0,f1	# ((double)a * 65536.0)/(double)b
	fctiwz	f1,f1		# convert result to int(which is really a fix)
	stfiwx	f1,0,r6
	lwz	r3,0(r6)
	addi	sp,sp,0x18
	blr

	.section .sdata2
	.balign 4
fix2double_c:
	.byte 0x61, 0x80, 0x00, 0x04
int2double_c:
	.byte 0x59, 0x80, 0x00, 0x04
