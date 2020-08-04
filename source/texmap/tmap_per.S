#include <ppc-asm.h>

# -------------------------------------------------------------
# Plot one pixel
# Lighting, No Transparency.
	.macro ppc_pix_l

	mr	eax, V0
	rlwimi	eax, U0, 26, 16, 21	# Now, eax has v:u, but it's 10 bits too high and garbage above it
	rlwinm	eax, eax, 22, 20, 31	# Shift right 10 bits, mask out high garbage (preserve only low 12 bits)
	
	lbzx	tr2, eax, es	# get source pixel

	add	U0, U0, edx	# u0 = u0 + du
	rlwimi	tr2, tr0, 24, 16, 23	# mask lighting value (%bh) above pixel value (%al)
	add	V0, V0, esi	# v0 = v0 + dv

	lbzx	eax, r3, tr2	# xlate lighting:pixel through lighting tables
	stbu	eax, 1(edi)	# {change this to stbu eax 1(edi) and kill the addi below}

	add	tr0, tr0, tr1	# fx_l += fx_dl_dx

	.endm

# -------------------------------------------------------------
# Plot one pixel
# Lighting & Transparency.
	.macro ppc_pix_lt

	mr	eax, V0
	rlwimi	eax, U0, 26, 16, 21	# Now, eax has v:u, but it's 10 bits too high and garbage above it
	rlwinm	eax, eax, 22, 20, 31	# Shift right 10 bits, mask out high garbage (preserve only low 12 bits)
	
	lbzx	tr2, eax, es	# get source pixel

	add	U0, U0, edx	# u0 = u0 + du
	cmpwi	tr2, 0xff
	add	V0, V0, esi	# v0 = v0 + dv

	beq	1f

	rlwimi	tr2, tr0, 24, 16, 23	# mask lighting value (%bh) above pixel value (%al)
	lbzx	eax, r3, tr2	# xlate lighting:pixel through lighting tables
	stb	eax, 0(edi)
1:	addi	edi, edi, 1
	add	tr0, tr0, tr1	# fx_l += fx_dl_dx
	.endm

# -------------------------------------------------------------
# Plot one pixel
# No Lighting & Transparency.
	.macro ppc_pix_t

	mr	eax, V0
	rlwimi	eax, U0, 26, 16, 21	# Now, eax has v:u, but it's 10 bits too high and garbage above it
	rlwinm	eax, eax, 22, 20, 31	# Shift right 10 bits, mask out high garbage (preserve only low 12 bits)
	
	lbzx	tr2, eax, es	# get source pixel

	add	U0, U0, edx	# u0 = u0 + du
	cmpwi	tr2, 0xff
	add	V0, V0, esi	# v0 = v0 + dv

	beq	1f

	stb	tr2, 0(edi)
1:	addi	edi, edi, 1
	.endm

# -------------------------------------------------------------
# Plot one pixel
# No Lighting or Transparency.
	.macro ppc_pix
	
	mr	eax, V0
	rlwimi	eax, U0, 26, 16, 21	# Now, eax has v:u, but it's 10 bits too high and garbage above it
	rlwinm	eax, eax, 22, 20, 31	# Shift right 10 bits, mask out high garbage (preserve only low 12 bits)
	
	add	U0, U0, edx	# u0 = u0 + du
	lbzx	r6, eax, es	# get source pixel

	add	V0, V0, esi	# v0 = v0 + dv
	stbu	r6, 1(edi)	#
	.endm
		
FUNC_START(asm_tmap_scanline_per)

#include	"tmap.inc"

# We don't pass any parameters, so we use those registers.
.equ r_num_left_over, 4

# PowerPC equates for 80x86 compatibility
.equ eax, r7
.equ ebx, r8
.equ ecx, r9
.equ edx, r10
.equ ebp, r11
.equ esi, r12
.equ edi, r19

.equ es, r20

.equ tr0, r21
.equ tr1, r22
.equ tr2, r23

.equ r_loop_count, r24

.equ U0, r25
.equ U1, r26
.equ V0, r27
.equ V1, r28

.equ DU1, r29
.equ DV1, r30
.equ DZ1, r31

#MWA	csect	texmap

# --------------------------------------------------------------------------------------------------
# Enter:
#	_xleft	fixed point left x coordinate
#	_xright	fixed point right x coordinate
#	_y	fixed point y coordinate
#	_pixptr	address of source pixel map
#	_u	fixed point initial u coordinate
#	_v	fixed point initial v coordinate
#	_z	fixed point initial z coordinate
#	_du_dx	fixed point du/dx
#	_dv_dx	fixed point dv/dx
#	_dz_dx	fixed point dz/dx

#   for (x = (int) xleft# x <= (int) xright# x++) {
#      _setcolor(read_pixel_from_tmap(srcb,((int) (u/z)) & 63,((int) (v/z)) & 63))#
#      _setpixel(x,y)#
#
#      u += du_dx#
#      v += dv_dx#
#      z += dz_dx#
#   }

#asm_tmap_scanline_per_:

.equ stack_space, 56
# Prolog
# 1. Store the current stack pointer and decrement the stack pointer by the size of the stack frame
# 3. Save the contents of the nonvolatile general-purpose registers to be used.
# 2. Save the contents of the Link Register

# PROLOGUE - called routine's responsibilities
	stwu	sp, -stack_space(sp)
	mflr	r0					# Get link register
	stw	r31, stack_space-4(sp)
	stw	r30, stack_space-8(sp)
	stw	r29, stack_space-12(sp)
	stw	r28, stack_space-16(sp)
	stw	r27, stack_space-20(sp)
	stw	r26, stack_space-24(sp)
	stw	r25, stack_space-28(sp)
	stw	r24, stack_space-32(sp)
	stw	r23, stack_space-36(sp)
	stw	r22, stack_space-40(sp)
	stw	r21, stack_space-44(sp)
	stw	r20, stack_space-48(sp)
	stw	r19, stack_space-52(sp)
	stw	r0, stack_space+4(sp)
									# might have saved stuff

#---------------------------- setup for loop ---------------------------------
# Setup for loop:	_loop_count  iterations = (int) xright - (int) xleft
#	esi	source pixel pointer = pixptr
#	edi	initial row pointer = y*320+x
# NOTE: fx_xright and fx_xleft changed from fix to int by mk on 12/01/94.

	lwz     es, pixptr@sdarel(r13)

# set edi = address of first pixel to modify
	lwz	edi, fx_y@sdarel(r13)
	lis	r3, y_pointers@h
	slwi	edi, edi, 2	#	mov	edi,_y_pointers[edi*4]

	lwz	ebx, fx_xleft@sdarel(r13)
	ori	r3, r3, y_pointers@l
	mr.	ebx, ebx	#	test	ebx, ebx
	lwzx	edi, r3, edi

	bgt	ebx_ok	#	jns	ebx_ok
	xor	ebx, ebx, ebx	#	xor	ebx, ebx
ebx_ok:
	add	edi, edi, ebx	#	add	edi,ebx
	lwz	tr0, write_buffer@sdarel(r13)
	lwz	eax, fx_xright@sdarel(r13)

	add	edi, edi, tr0
	
# set _loop_count = # of iterations
	sub.	eax, eax, ebx	#	sub	eax,ebx
	mr	r_loop_count, eax
	blt	_none_to_do	#	js	_none_to_do

# set initial values
	lwz	ebx, fx_u@sdarel(r13)
	lwz	ebp, fx_v@sdarel(r13)
	lwz	ecx, fx_z@sdarel(r13)
	lwz	tr0, per2_flag@sdarel(r13)
	lwz	DV1, fx_dv_dx@sdarel(r13)
	lwz	DU1, fx_du_dx@sdarel(r13)
	mr.	tr0, tr0
	lwz	DZ1, fx_dz_dx@sdarel(r13)
	beq	tmap_slow	#	je	tmap_loop

	lwz	tr0, Lighting_on@sdarel(r13)
  	mr.	tr0, tr0
	beq	tmap_loop_fast_nolight	#	je	tmap_loop_fast_nolight
  	b	tmap_loop_fast	#	jmp	tmap_loop_fast

#================ PERSPECTIVE TEXTURE MAP INNER LOOPS ========================
#
# Usage in loop:	eax	division, pixel value
#	ebx	u
#	ecx	z
#	edx	division
#	ebp	v
#	esi	source pixel pointer
#	edi	destination pixel pointer

#-------------------- NORMAL PERSPECTIVE TEXTURE MAP LOOP -----------------

tmap_slow_from_fast:
	mr	r_loop_count, r_num_left_over
	b	tmap_slow_0

tmap_slow:
	lis	r3, gr_fade_table@h	# r3 is fade table pointer
	lwz	esi, fx_l@sdarel(r13)
	lwz	tr2, fx_dl_dx@sdarel(r13)
	ori	r3, r3, gr_fade_table@l

tmap_slow_0:
	lwz	tr0, Transparency_on@sdarel(r13)
	mr.	tr0, tr0

	beq	tmap_slow_no_transparency

# ---------- Yes transparency.  Yes lighting. ----------
tmap_loop0:
	divw	tr0, ebp, ecx	# compute v coordinate
	divw	tr1, ebx, ecx	# compute u coordinate
	andi.	tr1, tr1, 0x3f
	rlwimi	tr1, tr0, 6, 20, 25
	lbzx	tr1, es, tr1	# mov	al, es:[ebx]	# get pixel from source bitmap

	rlwimi	tr1, esi, 24, 16, 23	# mask lighting value (%bh) above pixel value (%al)
	lbzx	tr1, r3, tr1	# xlate lighting:pixel through lighting tables
	add	esi, esi, tr2	# update lighting value

	cmpwi	cr0, tr1, 0xff	#	check for transparency
	beq	skip1	#	je	skip1

	stb	tr1, 0(edi)	#	mov	[edi],al
skip1:	addi	edi, edi, 1	# inc	edi
	
	add.	ecx, ecx, DZ1	#	add	ecx,_fx_dz_dx
	add	ebp, ebp, DV1	#	add	ebp,_fx_dv_dx
	add	ebx, ebx, DU1	# add	esi,_fx_du_dx
	beq	_div_0_abort	# je	_div_0_abort	# would be dividing by 0, so abort

	addic.	r_loop_count, r_loop_count, -1	# dec	_loop_count
	bge	tmap_loop0	# jns	tmap_loop0

# We detected a z=0 condition, which seems pretty bogus, don't you think?
# So, we abort, but maybe we want to know about it.
_div_0_abort:
_none_to_do:	
#  added by allender 6/21/95 -- from MPW example and PPC developers book
# EPILOGUE - return sequence		
		lwz	r0, stack_space+4(sp)
		lwz	r31, stack_space-4(sp)
		lwz	r30, stack_space-8(sp)
		lwz	r29, stack_space-12(sp)
		lwz	r28, stack_space-16(sp)
		lwz	r27, stack_space-20(sp)
		lwz	r26, stack_space-24(sp)
		lwz	r25, stack_space-28(sp)
		lwz	r24, stack_space-32(sp)
		lwz	r23, stack_space-36(sp)
		lwz	r22, stack_space-40(sp)
		lwz	r21, stack_space-44(sp)
		lwz	r20, stack_space-48(sp)
		lwz	r19, stack_space-52(sp)
		mtlr	r0						# Reset the link register
		addi	sp, sp, stack_space
		blr								# return via the link register

# ---------- No transparency.  Yes lighting. ----------

tmap_slow_no_transparency:
	divw	tr0, ebp, ecx	# compute v coordinate
	divw	tr1, ebx, ecx	# compute u coordinate
	andi.	tr1, tr1, 0x3f	# get u coordinate in 0..63
	rlwimi	tr1, tr0, 6, 20, 25	# pack together v:u in low 12 bits
	lbzx	tr1, es, tr1	# Read source pixel.
	rlwimi	tr1, esi, 24, 16, 23	# mask lighting value (%bh) above pixel value (%al)
	lbzx	tr1, r3, tr1	# xlate lighting:pixel through lighting tables
	add	esi, esi, tr2	# update lighting value

	stb	tr1, 0(edi)	#	mov	[edi],al
	addi	edi, edi, 1	# inc	edi
	
	add.	ecx, ecx, DZ1	#	add	ecx,_fx_dz_dx
	add	ebp, ebp, DV1	#	add	ebp,_fx_dv_dx
	add	ebx, ebx, DU1	# add	esi,_fx_du_dx
	beq	_div_0_abort	# je	_div_0_abort	# would be dividing by 0, so abort

	addic.	r_loop_count, r_loop_count, -1	# dec	_loop_count
	bge	tmap_slow_no_transparency	# jns	tmap_loop0
	b	_none_to_do

#-------------------------- PER/4 TMAPPER ----------------
# 
#	x = x1
#	U0 = u/w# V0 = v/w#
#	while ( 1 )
#		u += du_dx*4# v+= dv_dx*4
#		U1 = u/w# V1 = v/w#
#		DUDX = (U1-U0)/4# DVDX = (V1-V0)/4#
#
#	# Pixel 0
#		pixels = texmap[V0*64+U0]#
#		U0 += DUDX# V0 += DVDX
#	# Pixel 1
#		pixels = (pixels<<8)+texmap[V0*64+U0]#
#		U0 += DUDX# V0 += DVDX
#	# Pixel 2
#		pixels = (pixels<<8)+texmap[V0*64+U0]#
#		U0 += DUDX# V0 += DVDX
#	# Pixel 3
#		pixels = (pixels<<8)+texmap[V0*64+U0]#
#
#		screen[x] = pixel
#		x += 4#
#		U0 = U1# V0 = V1 

# Note: If you change NBITS, you must change the number of calls to the ppc_pix macros.
.equ NBITS, 4	# 2^NBITS pixels plotted per divide
.equ NBITS_mask, 15	# 2^NBITS-1
.equ NBITS_shl_minus_2, 4	# 2 ^ (NBITS-2)
.equ ZSHIFT, 4	# precision used in PDIV macro
.equ DIV_SHIFT, 4	# Used to be 8...overflowed, smaller less likely to overflow

# -------------------------------------- Start of Getting Dword Aligned ----------------------------------------------
#	ebx	fx_u
#	ebp	fx_v
#	esi	fx_l
#	r3	gr_fade_table
tmap_loop_fast:
	lis	r3, gr_fade_table@h
	lwz	esi, fx_l@sdarel(r13)
	ori	r3, r3, gr_fade_table@l
	lwz	tr2, fx_dl_dx@sdarel(r13)

# This is a hack!  If we allow zero pixels to be plotted for alignment, the code later hangs.
#	andi.	tr0, edi, 3	# DEBUG HACK!!
#	beq	skip_test	# DEBUG HACK!!

NotDwordAligned1:
	andi.	tr0, edi, 3	#	test	edi, 11b
	beq	DwordAligned1	#	jz	DwordAligned1
skip_test:			# DEBUG HACK!!
	divw	tr0, ebp, ecx	# tr0: v coodinate
	divw	tr1, ebx, ecx	# tr1: u coordinate
	rlwimi	tr1, tr0, 6, 20, 25	# get v:u in low 12 bits, but garbage above
	andi.	tr1, tr1, 0xfff	# preserve only 12 bit index

	lbzx	tr1, es, tr1	# mov	al, es:[ebx]	# get pixel from source bitmap

	rlwimi	tr1, esi, 24, 16, 23	# mask lighting value (%bh) above pixel value (%al)
	lbzx	tr0, tr1, r3	# xlate lighting:pixel through lighting tables

	cmpwi	cr0, tr0, 0xff	# transparent pixel?
	beq	skip2	#	yes, skip

	stb	tr0, 0(edi)	#	mov	[edi],al
skip2:	addi	edi, edi, 1	# inc	edi
	
# update deltas
	add	esi, esi, tr2	# 	fx_l += fx_dl_dx
	add.	ecx, ecx, DZ1	#	add	ecx,_fx_dz_dx
	add	ebx, ebx, DU1	#	add	esi,_fx_du_dx
	add	ebp, ebp, DV1	#	add	ebp,_fx_dv_dx
	beq	_div_0_abort	#	je	_div_0_abort	# would be dividing by 0, so abort

	addic.	r_loop_count, r_loop_count, -1	# dec	_loop_count
	bge	NotDwordAligned1

	b	_none_to_do

# -------------------------------------- End of Getting Dword Aligned ----------------------------------------------

DwordAligned1:
	addi	r_loop_count, r_loop_count, 1
	andi.	r_num_left_over, r_loop_count, NBITS_mask
	srwi.	r_loop_count, r_loop_count, NBITS
	beq	tmap_slow_from_fast

# compute initial u, v coordinates
	slwi	eax, ebp, DIV_SHIFT
	divw	V0, eax, ecx
	slwi	V0, V0, 16 - DIV_SHIFT

	slwi	eax, ebx, DIV_SHIFT
	divw	U0, eax, ecx
	slwi	U0, U0, 16 - DIV_SHIFT

# Set deltas to NPIXS pixel increments
	lwz	DU1, fx_du_dx@sdarel(r13)
	lwz	DV1, fx_dv_dx@sdarel(r13)
	lwz	DZ1, fx_dz_dx@sdarel(r13)

# LIGHTING CODE
	#lwz	tr0, fx_l@sdarel(r13)
	mr	tr0, esi
	#lwz	tr1, fx_dl_dx@sdarel(r13)
	mr	tr1, tr2

	slwi	DU1, DU1, NBITS
	slwi	DV1, DV1, NBITS
	slwi	DZ1, DZ1, NBITS


# Inside this loop, tr0 = fx_l, tr1 = fx_dl_dx

TopOfLoop4:
	add.	ecx, ecx, DZ1	# add	ecx, DZ1
	add	ebx, ebx, DU1	# add	ebx, DU1
	add	ebp, ebp, DV1	# add	ebp, DV1
	beq	_div_0_abort	# would be dividing by 0, so abort

# Find fixed U1, V1
	slwi	eax, ebx, DIV_SHIFT
	divw	U1, eax, ecx

	slwi	eax, ebp, DIV_SHIFT
	divw	V1, eax, ecx

	slwi	U1, U1, 16 - DIV_SHIFT
	slwi	V1, V1, 16 - DIV_SHIFT

# PPC: Make %esi be dv, %edx be du
	sub	esi, V1, V0
	sub	edx, U1, U0
	lwz	eax, Transparency_on@sdarel(r13)
	srawi	esi, esi, NBITS
	mr.	eax, eax
	srawi	edx, edx, NBITS

	li	r5, 16	# do 16 pixels... (2^NBITS)
	mtctr	r5
	bne	yes_trans1	# je	no_trans1

	addi	edi, edi, -1
pix_loop1:	
	ppc_pix_l
	bdnz	pix_loop1
	addi	edi, edi, 1
cont1:
	addic.	r_loop_count, r_loop_count, -1
	mr	U0, U1
	mr	V0, V1
	bne	TopOfLoop4

EndOfLoop4:
	mr.	r_num_left_over, r_num_left_over
	beq	_none_to_do
	b	DoEndPixels

# -------------------------------------------------------
yes_trans1:
	ppc_pix_lt
	bdnz	yes_trans1
	b	cont1

# ----------------------------------------- Start of LeftOver Pixels ------------------------------------------
DoEndPixels:
# This is the stuff to determine whether to use the slower, but more accurate, leftover pixel stuff.

	add.	ecx, ecx, DZ1	# add	ecx, DZ1
	add	ebx, ebx, DU1	#	add	ebx, DU1
	add	ebp, ebp, DV1	# add	ebp, DV1
	beq	_div_0_abort	# je	_div_0_abort
	bgt	dep_cont	# jns	dep_cont

# z went negative.
# this can happen because we added DZ1 to the current z, but dz1 represents dz for perhaps 16 pixels
# though we might only plot one more pixel.

# Instead of converting the ugly code below, I'm just going to abort if z went negative.
# It hardly ever does and we shipped shareware that way...

  b _none_to_do

dep_cont:
	slwi	eax, ebx, DIV_SHIFT
	divw	U1, eax, ecx
	slwi	U1, U1, 16 - DIV_SHIFT

	slwi	eax, ebp, DIV_SHIFT
	divw	V1, eax, ecx
	slwi	V1, V1, 16 - DIV_SHIFT

	sub	esi, V1, V0
	sub	edx, U1, U0
	srawi	esi, esi, NBITS
	srawi	edx, edx, NBITS

leftover_loop:	mr	eax, V0
	rlwimi	eax, U0, 26, 16, 21	# Now, eax has v:u, but it's 10 bits too high and garbage above it
	rlwinm	eax, eax, 22, 20, 31	# Shift right 10 bits, mask out high garbage (preserve only low 12 bits)
	
	lbzx	tr2, eax, es	# get source pixel

	add	U0, U0, edx	# u0 = u0 + du
	add	V0, V0, esi	# v0 = v0 + dv
	cmpwi	cr0, tr2, 0xff	# transparent pixel?

	beq	skipa1

	rlwimi	tr2, tr0, 24, 16, 23	# mask lighting value (%bh) above pixel value (%al)
	lbzx	eax, r3, tr2	# xlate lighting:pixel through lighting tables
	stb	eax, 0(edi)
skipa1:	addi	edi, edi, 1
	add	tr0, tr0, tr1	# fx_l += fx_dl_dx

 	addic.	r_num_left_over, r_num_left_over, -1
	bne	leftover_loop

	b	_none_to_do	# jmp	_none_to_do

nol_tmap_slow_from_fast:
	mr	r_loop_count, r_num_left_over

# ---------- Yes transparency.  No lighting. ----------
# (Note: We don't know for sure there is lighting, but, except for debugging, if we aren't lighting, we _do_ have transparency.)
nol_tmap_slow_NoLight:
nol_tmap_loop0_nolight:
	divw	tr0, ebp, ecx	# compute v coordinate
	divw	tr1, ebx, ecx	# compute u coordinate
	andi.	tr1, tr1, 0x3f
	rlwimi	tr1, tr0, 6, 20, 25
	lbzx	tr1, es, tr1	# mov	al, es:[ebx]	# get pixel from source bitmap

	cmpwi	cr0, tr1, 0xff	#	check for transparency
	beq	nol_skip1a	#	je	skip1

	stb	tr1, 0(edi)	#	mov	[edi],al
nol_skip1a:	addi	edi, edi, 1	# inc	edi
	
	add.	ecx, ecx, DZ1	#	add	ecx,_fx_dz_dx
	add	ebp, ebp, DV1	#	add	ebp,_fx_dv_dx
	add	ebx, ebx, DU1	# add	esi,_fx_du_dx
	beq	_div_0_abort	# je	_div_0_abort	# would be dividing by 0, so abort

	addic.	r_loop_count, r_loop_count, -1	# dec	_loop_count
	bge	nol_tmap_loop0_nolight	# jns	tmap_loop0

	b	_none_to_do

# -------------------------------------- Start of Getting Dword Aligned ----------------------------------------------
#	ebx	fx_u
#	ebp	fx_v
#	esi	fx_l
#	r3	gr_fade_table
tmap_loop_fast_nolight:

# This is a hack!  If we allow zero pixels to be plotted for alignment, the code later hangs.
#	andi.	tr0, edi, 3	# DEBUG HACK!!
#	beq	nol_skip_test	# DEBUG HACK!!

nol_NotDwordAligned1:
	andi.	tr0, edi, 3	#	test	edi, 11b
	beq	nol_DwordAligned1	#	jz	DwordAligned1
nol_skip_test:			# DEBUG HACK!!
	divw	tr0, ebp, ecx	# tr0: v coodinate
	divw	tr1, ebx, ecx	# tr1: u coordinate
	rlwimi	tr1, tr0, 6, 20, 25	# get v:u in low 12 bits, but garbage above
	andi.	tr1, tr1, 0xfff	# preserve only 12 bit index

	lbzx	tr1, es, tr1	# mov	al, es:[ebx]	# get pixel from source bitmap

	cmpwi	cr0, tr1, 0xff	# transparent pixel?
	beq	nol_skip2	#	yes, skip

	stb	tr1, 0(edi)	#	mov	[edi],al
nol_skip2:	addi	edi, edi, 1	# inc	edi
	
# update deltas
	add.	ecx, ecx, DZ1	#	add	ecx,_fx_dz_dx
	add	ebx, ebx, DU1	#	add	esi,_fx_du_dx
	add	ebp, ebp, DV1	#	add	ebp,_fx_dv_dx
	beq	_div_0_abort	#	je	_div_0_abort	# would be dividing by 0, so abort

	addic.	r_loop_count, r_loop_count, -1	# dec	_loop_count
	bge	nol_NotDwordAligned1

	b	_none_to_do

# -------------------------------------- End of Getting Dword Aligned ----------------------------------------------

nol_DwordAligned1:
	addi	r_loop_count, r_loop_count, 1
	andi.	r_num_left_over, r_loop_count, NBITS_mask
	srwi.	r_loop_count, r_loop_count, NBITS
	beq	nol_tmap_slow_from_fast

# compute initial u, v coordinates
	slwi	eax, ebp, DIV_SHIFT
	divw	V0, eax, ecx
	slwi	V0, V0, 16 - DIV_SHIFT

	slwi	eax, ebx, DIV_SHIFT
	divw	U0, eax, ecx
	slwi	U0, U0, 16 - DIV_SHIFT

# Set deltas to NPIXS pixel increments
	lwz	DU1, fx_du_dx@sdarel(r13)
	lwz	DV1, fx_dv_dx@sdarel(r13)
	lwz	DZ1, fx_dz_dx@sdarel(r13)
	slwi	DU1, DU1, NBITS
	slwi	DV1, DV1, NBITS
	slwi	DZ1, DZ1, NBITS

nol_TopOfLoop4:
	add.	ecx, ecx, DZ1	# add	ecx, DZ1
	add	ebx, ebx, DU1	#	add	ebx, DU1
	add	ebp, ebp, DV1	# add	ebp, DV1
	beq	_div_0_abort	# would be dividing by 0, so abort

# Find fixed U1, V1
	slwi	eax, ebx, DIV_SHIFT
	divw	U1, eax, ecx

	slwi	eax, ebp, DIV_SHIFT
	divw	V1, eax, ecx

	slwi	U1, U1, 16 - DIV_SHIFT
	slwi	V1, V1, 16 - DIV_SHIFT

# PPC: Make %esi be dv, %edx be du
	sub	esi, V1, V0
	lwz	eax, Transparency_on@sdarel(r13)
	sub	edx, U1, U0
	srawi	esi, esi, NBITS
	mr.	eax, eax
	srawi	edx, edx, NBITS

# Plot 16 pixels. (2^NBITS)
	li	r5, 16	# do 16 times...
	mtctr	r5

	bne	nol_yes_trans1	# je	no_trans1
	addi	edi, edi, -1

# -----------------------------------------------
nol_pix_loop1:
	ppc_pix
	bdnz	nol_pix_loop1
	addi	edi, edi, 1
	
nol_cont1:
	addic.	r_loop_count, r_loop_count, -1
	mr	U0, U1
	mr	V0, V1
	bne	nol_TopOfLoop4

nol_EndOfLoop4:
	mr.	r_num_left_over, r_num_left_over
	beq	_none_to_do
	b	nol_DoEndPixels

# -------------------------------------------------------
nol_yes_trans1:
# Plot 16 pixels. (2^NBITS)
	ppc_pix_t
	bdnz nol_yes_trans1
	b	nol_cont1

# ----------------------------------------- Start of LeftOver Pixels ------------------------------------------
nol_DoEndPixels:
# This is the stuff to determine whether to use the slower, but more accurate, leftover pixel stuff.

	add.	ecx, ecx, DZ1	# add	ecx, DZ1
	add	ebx, ebx, DU1	#	add	ebx, DU1
	add	ebp, ebp, DV1	# add	ebp, DV1
	beq	_div_0_abort	# je	_div_0_abort
	blt	_none_to_do

	slwi	eax, ebx, DIV_SHIFT
	divw	U1, eax, ecx
	slwi	U1, U1, 16 - DIV_SHIFT

	slwi	eax, ebp, DIV_SHIFT
	divw	V1, eax, ecx
	slwi	V1, V1, 16 - DIV_SHIFT

	sub	esi, V1, V0
	sub	edx, U1, U0
	srawi	esi, esi, NBITS
	srawi	edx, edx, NBITS

nol_leftover_loop:	mr	eax, V0
	rlwimi	eax, U0, 26, 16, 21	# Now, eax has v:u, but it's 10 bits too high and garbage above it
	rlwinm	eax, eax, 22, 20, 31	# Shift right 10 bits, mask out high garbage (preserve only low 12 bits)
	
	lbzx	tr2, eax, es	# get source pixel

	add	U0, U0, edx	# u0 = u0 + du
	add	V0, V0, esi	# v0 = v0 + dv
	cmpwi	cr0, tr2, 0xff	# transparent pixel?

	beq	nol_skipa1

	stb	tr2, 0(edi)
nol_skipa1:	addi	edi, edi, 1

 	addic.	r_num_left_over, r_num_left_over, -1
	bne	nol_leftover_loop

	b	_none_to_do	# jmp	_none_to_do
