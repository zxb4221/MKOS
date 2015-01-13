ASM							= nasm

all : bootMBR.bin

bootMBR.bin : ./bootMBR.asm
	$(ASM) -o ./bootMBR.bin ./bootMBR.asm
