#!/bin/bash

nasm -f elf tiny2.asm && ld -s -o tiny2 tiny2.o
