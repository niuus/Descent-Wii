#include <ppc-asm.h>

#define GQR0_OFFSET 148
#define GQR1_OFFSET 152
#define GQR2_OFFSET 156
#define GQR3_OFFSET 160
#define GQR4_OFFSET 164
#define GQR5_OFFSET 168
#define GQR6_OFFSET 172
#define GQR7_OFFSET 176

#define EXCPT_OFFSET 8
#define SRR0_OFFSET 12
#define SRR1_OFFSET 16
#define GPR0_OFFSET 20
#define GPR1_OFFSET 24
#define GPR2_OFFSET 28
#define GPR3_OFFSET 32
#define GPR4_OFFSET 36
#define GPR5_OFFSET 40
#define GPR6_OFFSET 44
#define GPR7_OFFSET 48
#define GPR8_OFFSET 52
#define GPR9_OFFSET 56
#define GPR10_OFFSET 60
#define GPR11_OFFSET 64
#define GPR12_OFFSET 68
#define GPR13_OFFSET 72
#define GPR14_OFFSET 76
#define GPR15_OFFSET 80
#define GPR16_OFFSET 84
#define GPR17_OFFSET 88
#define GPR18_OFFSET 92
#define GPR19_OFFSET 96
#define GPR20_OFFSET 100
#define GPR21_OFFSET 104
#define GPR22_OFFSET 108
#define GPR23_OFFSET 112
#define GPR24_OFFSET 116
#define GPR25_OFFSET 120
#define GPR26_OFFSET 124
#define GPR27_OFFSET 128
#define GPR28_OFFSET 132
#define GPR29_OFFSET 136
#define GPR30_OFFSET 140
#define GPR31_OFFSET 144

#define CR_OFFSET	 180
#define LR_OFFSET	 184
#define CTR_OFFSET	 188
#define XER_OFFSET	 192

#define EXCEPTION_FRAME_END 728


#define EXCEPTION_PROLOG		\
	stw      r6,GPR6_OFFSET(sp);	\
	stw      r7,GPR7_OFFSET(sp);	\
	stw      r8,GPR8_OFFSET(sp);	\
	stw      r9,GPR9_OFFSET(sp);	\
	stw      r10,GPR10_OFFSET(sp);	\
	stw      r11,GPR11_OFFSET(sp);	\
	stw      r12,GPR12_OFFSET(sp)
	
#define EXCEPTION_EPILOG			\
	lwz		r12,GPR12_OFFSET(sp);	\
	lwz		r11,GPR11_OFFSET(sp);	\
	lwz		r10,GPR10_OFFSET(sp);	\
	lwz		r9,GPR9_OFFSET(sp);	\
	lwz		r8,GPR8_OFFSET(sp);	\
	lwz		r7,GPR7_OFFSET(sp);	\
	lwz		r6,GPR6_OFFSET(sp);	\
	lwz		r5,GPR5_OFFSET(sp);	\
	lwz		r4,GPR4_OFFSET(sp);	\
	lwz		r3,GPR3_OFFSET(sp)

	.extern		my_exceptionhandler
	.globl		descent_exceptionhandler
descent_exceptionhandler:
	stwu		sp,-EXCEPTION_FRAME_END(sp)		//now we're able to adjust the stackpointer with it's cached address

	EXCEPTION_PROLOG

	// store the non-volatile registers if this isn't a trap exception
	lwz		r0,EXCPT_OFFSET(sp)
	cmpwi		r0,0x06
	bne		1f
	lwz		r0,SRR1_OFFSET(sp)
	andis.		r0,r0,2
	bne		2f
1:
	stmw		r13,GPR13_OFFSET(sp)
2:
	addi		r3,sp,0x08
	bl		my_exceptionhandler

	lwz		r4,CR_OFFSET(sp)
	mtcr		r4
	lwz		r4,LR_OFFSET(sp)
	mtlr		r4
	lwz		r4,CTR_OFFSET(sp)
	mtctr		r4
	lwz		r4,XER_OFFSET(sp)
	mtxer		r4

	lwz		toc,GPR2_OFFSET(sp)
	lwz		r0,GPR0_OFFSET(sp)

	lwz		r4,SRR0_OFFSET(sp)
	mtsrr0		r4
	lwz		r4,SRR1_OFFSET(sp)
	mtsrr1		r4

	EXCEPTION_EPILOG

	addi		sp,sp,EXCEPTION_FRAME_END
	rfi
