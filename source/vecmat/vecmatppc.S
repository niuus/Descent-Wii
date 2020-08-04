#include <ppc-asm.h>

# vm_vec_dotprod
#
# on entry:
#		r3 -- v0
#		r4 -- v1
#
# r9 and r10 store intermediate running sum
#
# return dotproduct in r3

FUNC_START(vm_vec_dotprod)
	lwz	 r5, 0(r3)		# v0.x
	lwz	 r6, 0(r4)		# v1.x
	mullw	 r9, r5, r6		# multiply low word
	mulhw	r10, r5, r6		# multiply high word (signed)

	lwz	 r5, 4(r3)		# v0.y
	lwz	 r6, 4(r4)		# v1.y
	mullw	 r7, r5, r6		# multiply low word
	mulhw	 r8, r5, r6		# multiply high word (signed)
	lwz	 r5, 8(r3)		# v0.z
	lwz	 r6, 8(r4)		# v1.z
	addc	 r9, r9, r7		# add low words together setting carry bit
	adde	r10,r10, r8		# add high words with the carry bit

	mullw	 r3, r5, r6		# multiply low word
	mulhw	 r4, r5, r6		# multiply high word (signed)
	addc	 r9, r9, r3		# add low words together setting carry bit
	adde	r10,r10, r4		# add high words with the carry bit
	
	srwi	 r3, r9,16		# mask off low portion of result
	rlwimi	 r3,r10,16, 0,15	# insert high portion of result

	blr				# return to caller

# fix vm_vec_dot3(fix x,fix y,fix z,vms_vector *v)
#
# on entry:
#		r3 -- x
#		r4 -- y
#		r5 -- z
#		r6 -- v
#
# r10 and r11 store intermediate running sum
#
# returns dotprod in r3

FUNC_START(vm_vec_dot3)
	lwz	 r7, 0(r6)		# v.x
	mullw	r10, r7, r3		
	mulhw	r11, r7, r3
	
	lwz	 r7, 4(r6)		# v.y
	mullw	 r8, r7, r4
	mulhw	 r9, r7, r4
	lwz	 r7, 8(r6)		# v.z
	addc	r10,r10, r8		# add low words together setting carry bit
	adde	r11,r11, r9		# add high words with the carry bit
	
	mullw	 r3, r7, r5
	mulhw	 r4, r7, r5
	addc	r10,r10, r3		# add low words together setting carry bit
	adde	r11,r11, r4		# add high words with the carry bit

	srwi	 r3,r10,16		# mask off low portion of result
	rlwimi	 r3,r11,16, 0,15	# insert high portion of result

	blr				# return to caller

# vms_vector *vm_vec_crossprod(vms_vector *dest,vms_vector *src0,vms_vector *src1)
#
# on entry:
#
#		r3 -- destination vector
#		r4 -- src0
#		r5 -- src1
#
# r3 contains result of crossprod as return value

FUNC_START(vm_vec_crossprod)
#	q.low = q.high = 0;
#	fixmulaccum(&q,src0->y,src1->z);
#	fixmulaccum(&q,-src0->z,src1->y);
#	dest->x = fixquadadjust(&q);

	lwz	 r7, 4(r4)		# src0->y
	lwz	r12, 4(r5)		# src1->y
	lwz	 r8, 8(r4)		# src0->z
	lwz	 r6, 8(r5)		# src1->z

	mullw	 r9, r7, r6		# multiply low word (src0->y * src1->z)
	mulhw	r10, r7, r6		# multiply high word (signed)
	mullw	r11,r12,r8		# multiply low word (src0->z * src1->y)
	mulhw	r12,r12,r8		# multiply high word (signed)

	subfc	r11,r11, r9		# sub low words together setting carry bit
	subfe	r12,r12,r10		# sub high words with the carry bit

	srwi	 r9,r11,16		# mask off low portion of result
	rlwimi	 r9,r12,16, 0,15	# insert high portion of result
	stw	 r9, 0(r3)		# (src0->y * src1->z) - (src0->z * src1->y)
	
#	q.low = q.high = 0;
#	fixmulaccum(&q,src0->z,src1->x);
#	fixmulaccum(&q,-src0->x,src1->z);
#	dest->y = fixquadadjust(&q);

	lwz	r10, 0(r4)		# src0->x
	mr	 r4,  r7		# src0->y
	lwz	 r7, 0(r5)		# src1->x
	lwz	 r5, 4(r5)		# src1->y

	mullw	r11, r6,r10		# multiply low word (src0->x * src1->z)
	mulhw	r12, r6,r10		# multiply high word (signed)
	mullw	 r9, r8, r7		# multiply low word (src0->z * src1->x)
	mulhw	 r8, r8, r7		# multiply high word (signed)

	subfc	r11,r11, r9		# sub low words together setting carry bit
	subfe	r12,r12, r8		# sub high words with the carry bit

	srwi	 r6,r11,16		# mask off low portion of result
	rlwimi	 r6,r12,16, 0,15	# insert high portion of result
	stw	 r6, 4(r3)		# (src0->z * src1->x) - (src0->x * src1->z)

#	q.low = q.high = 0;
#	fixmulaccum(&q,src0->x,src1->y);
#	fixmulaccum(&q,-src0->y,src1->x);
#	dest->z = fixquadadjust(&q);

	mullw	 r9,r10, r5		# multiply low word (src0->x * src1->y)
	mulhw	r10,r10, r5		# multiply high word (signed)
	mullw	 r6, r4, r7		# multiply low word (src0->y * src1->x)
	mulhw	 r8, r4, r7		# multiply high word (signed)

	subfc	r11, r6, r9		# sub low words together setting carry bit
	subfe	r12, r8,r10		# sub high words with the carry bit

	srwi	 r5,r11,16		# mask off low portion of result
	rlwimi	 r5,r12,16, 0,15	# insert high portion of result
	stw	 r5, 8(r3)		# (src0->x * src1->y) - (src0->y * src1->x)

	blr
	


