extern exit
[SECTION text]
[BITS	32]


global _start
_start:
push eax;
push gs

mov ax,27
and ax,0xFFFC
or ax,3
mov gs,ax
mov ah,0ch
mov  byte al,[_ch]
mov [gs:((80*15+75)*2)],ax

pop gs
pop eax

call exit
jmp $	;never arrive here

[SECTION .data]
ALIGN	32
DataString:				db		"THIS IS DATA!"
_ch:                                 db       'Z'
times   10                    db "ww"
