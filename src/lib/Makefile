ASM	= nasm
CC = gcc
CFLAGS = -fno-stack-protector -g -c -m32  -fno-builtin -o
LIB=mkcrt.a
LOBJS=exit.o \
	  misc.o \
	  syscall.o \
	  string.o \
	  start.o \
	  printf.o \
	  vsprintf.o \

all:$(LIB)

$(LIB): $(LOBJS)
	ar -rv $@ $<

printf.o : printf.c
	$(CC) $(CFLAGS) $@ $<
vsprintf.o : vsprintf.c
	$(CC) $(CFLAGS) $@ $<
	
misc.o:misc.c
	$(CC) $(CFLAGS) $@ $<
syscall.o:syscall.asm
	$(ASM) -f elf -o $@ $<
string.o:string.asm
	$(ASM) -f elf -o $@ $<
exit.o : exit.c
	$(CC) $(CFLAGS) $@ $<
	
start.o : start.asm
	$(ASM) -f elf -o $@ $<