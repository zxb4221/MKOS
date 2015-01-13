[SECTION text]
[BITS	32]
global _start
_start:
int 0x80

jmp $

[SECTION .data]
ALIGN	32
DataString:				db		"THIS IS DATA!"
_ch:                                 db       'Z'
times   1024                    db "11"
