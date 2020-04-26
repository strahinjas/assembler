.section iv_table, "a"
.word ivt_entry0
.word ivt_entry1
.word ivt_entry2
.word ivt_entry3
.skip 8


.section routines, "ax"
.global ivt_entry0, ivt_entry1
.global ivt_entry2, ivt_entry3

.extern _start

# processor initialization
ivt_entry0:
	mov r0, 0
	mov r1, 0
	mov r2, 0
	mov r3, 0
	mov r4, 0
	mov r5, 0
	mov sp, 0xFF00		# stack start
	mov *0xFF10, 0		# reset timer_cfg
	mov psw, 0xE000		# allow all interrupts
	mov pc, &_start		# jump to address of the first instruction

# invalid instruction
ivt_entry1:
	mov *0xFF00, 73		# I
	mov *0xFF00, 110	# n
	mov *0xFF00, 118    # v
	mov *0xFF00, 97     # a
	mov *0xFF00, 108    # l
	mov *0xFF00, 105    # i
	mov *0xFF00, 100    # d
	mov *0xFF00, 32     #
	mov *0xFF00, 105    # i
	mov *0xFF00, 110    # n
	mov *0xFF00, 115    # s
	mov *0xFF00, 116    # t
	mov *0xFF00, 114    # r
	mov *0xFF00, 117    # u
	mov *0xFF00, 99     # c
	mov *0xFF00, 116    # t
	mov *0xFF00, 105    # i
	mov *0xFF00, 111    # o
	mov *0xFF00, 110    # n
	mov *0xFF00, 33     # !
	mov *0xFF00, 10     # \n
	halt

# timer interrupt routine
ivt_entry2:
#	mov *0xFF00, 84		# T
#	mov *0xFF00, 105	# i
#	mov *0xFF00, 109	# m
#	mov *0xFF00, 101	# e
#	mov *0xFF00, 114	# r
#	mov *0xFF00, 32		#
#	mov *0xFF00, 116	# t
#	mov *0xFF00, 105	# i
#	mov *0xFF00, 99		# c
#	mov *0xFF00, 107	# k
#	mov *0xFF00, 101	# e
#	mov *0xFF00, 100	# d
#	mov *0xFF00, 33		# !
#	mov *0xFF00, 10		# \n
#	mov *0xFF10, 3
	iret

# terminal interrupt routine
ivt_entry3:
	mov *0xFF00, *0xFF02
	mov *0xFF02, 0
	iret