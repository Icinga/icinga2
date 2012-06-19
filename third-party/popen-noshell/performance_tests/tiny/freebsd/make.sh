#!/bin/sh
nasm -f elf tiny.asm
ld -s -o tiny2 tiny.o
