.section global, "ax"
.skip 16

.text
_start:	popb r0h
.byte	0xAA00 - _start
.global _start
.end
this
content
is
irrelevant
***
assembler
will
ignore
it