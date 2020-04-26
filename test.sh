#!/bin/bash

echo directives.s
bin/assembler -o tests/directives.o tests/directives.s
echo equ.s
bin/assembler -o tests/equ.o tests/equ.s
echo equ_cycle.s
bin/assembler -o tests/equ_cycle.o tests/equ_cycle.s
echo addressing.s
bin/assembler -o tests/addressing.o tests/addressing.s
echo global.s
bin/assembler -o tests/global.o tests/global.s
echo jumps.s
bin/assembler -o tests/jumps.o tests/jumps.s
echo setup.s
bin/assembler -o tests/setup.o tests/setup.s
echo loop.s
bin/assembler -o tests/loop.o tests/loop.s