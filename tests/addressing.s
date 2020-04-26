.section addressing, "ax"

_start:
	mov	 r0, 0x0055
	movw r0, 0x0055		# same as above
	
	movb [r2],  a
	movb r2[0], a		# same as above
	
	mov *0x1234, &a
	mov r3, 0
	mov r3[0x1234], &a
	mov [r3]0x1234, &a	# same as above
	cmp c, b
	jgt _start			# absolute jump
	jmp $_start			# pc relative jump

.data					# "aw" are default flags
	.skip 16
a:	.word 0x1111
	.equ b, 10
	.equ c, 20

.end