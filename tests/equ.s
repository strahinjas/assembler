.global a, c

.data
.equ	a, 0xA
.equ	b, a + 1
.equ	c, b
.equ	d, main + a

.text
.skip	0xA
.align	3

_start:	movb r0l, d

.end