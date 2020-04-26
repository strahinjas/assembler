.global _start

.section jumps "awx"

_start:	pushb 1
		pushb 2
a:		pushw 1111
		pushw 2222
		call $printf
		cmp r0, 5
		jgt b
		push 36
		jmp a
b:		jeq a
		jne a
		ret

.end