TEST.ELF : test.asm
	nasm -f elf -o ./test.o ./test.asm
	nasm -f elf -o ./intcall.o ./intcall.asm
	ld -Ttext 0x1400000 -m elf_i386 -o ./TESTFILE test.o ../lib/mkcrt.a ../lib/misc.o ../lib/syscall.o ../lib/string.o
	ld -Ttext 0x1400000 -m elf_i386 -o ./TESTCALL intcall.o
