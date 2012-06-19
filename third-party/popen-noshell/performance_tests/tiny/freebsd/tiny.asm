; http://www.freebsd.org/doc/en/books/developers-handbook/x86-first-program.html

   %include    'system.inc'

section .data
hello   db  'Hello, world!', 0Ah
hbytes  equ $-hello

section .text
global  _start
_start:
push    dword hbytes
push    dword hello
push    dword stdout
sys.write

push    dword 0
sys.exit
