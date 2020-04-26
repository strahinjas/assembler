.global start

.section directives, "aw"

.skip	100
.byte	0xFF, 0x00, 0x05, 0xAA

.align	3

.word	0x1111, 0x2222, 0x3333, 0x4444, 0x5555

start:
.skip	200

end:
.word	start, end - start

.align	3

.end