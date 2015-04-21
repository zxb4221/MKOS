ASM = nasm
CC = gcc
CFLAGS = -fno-stack-protector -g -c -m32  -fno-builtin -o
LD = ld

OBJS=kernelex.o start.o global.o kliba.o stringAsm.o protect.o i8259.o klib.o main.o clock.o syscall.o proc.o \
	keyboard.o mouse.o tty.o ../lib/vsprintf.o ../lib/printf.o misc.o console.o systask.o hd.o fs.o mm.o gui.o string.o kmalloc.o memory.o \
	init.o shell.o task_mouse.o

all : kernelex.elf

kernelex.elf : $(OBJS)
	$(LD) -Ttext 0xA00000  -m elf_i386 -o ./kernelex.elf $(OBJS)

memory.o : ../lib/memory.c
	$(CC) $(CFLAGS) $@ $<
	
kmalloc.o : ../lib/kmalloc.c
	$(CC) $(CFLAGS) $@ $<

string.o : ./string.c
	$(CC) $(CFLAGS) $@ $<

shell.o : ../services/shell.c
	$(CC) $(CFLAGS) $@ $<
	
task_mouse.o : ../services/task_mouse.c
	$(CC) $(CFLAGS) $@ $<
	
init.o : ../services/init.c
	$(CC) $(CFLAGS) $@ $<
	
hd.o : ../services/hd.c
	$(CC) $(CFLAGS) $@ $<

fs.o : ../services/fs.c
	$(CC) $(CFLAGS) $@ $<

mm.o : ../services/mm.c
	$(CC) $(CFLAGS) $@ $<
gui.o : ../services/gui.c
	$(CC) $(CFLAGS) $@ $<
	
systask.o : ../services/systask.c
	$(CC) $(CFLAGS) $@ $<

console.o : ../services/console.c
	$(CC) $(CFLAGS) $@ $<

misc.o : ./misc.c
	$(CC) $(CFLAGS) $@ $<
	
tty.o : ../services/tty.c
	$(CC) $(CFLAGS) $@ $<

mouse.o : ./mouse.c
	$(CC) $(CFLAGS) $@ $<
	
keyboard.o : ./keyboard.c
	$(CC) $(CFLAGS) $@ $<

proc.o : ./proc.c
	$(CC) $(CFLAGS) $@ $<

syscall.o : ./syscall.asm
	$(ASM) -f elf -o $@ $<

clock.o : ./clock.c
	$(CC) $(CFLAGS) $@ $<

main.o : ./main.c
	$(CC) $(CFLAGS) $@ $<

kernelex.o : ./kernelex.asm
	$(ASM) -f elf -o $@ $<

start.o : ./start.c
	$(CC) $(CFLAGS) $@ $<

global.o : ./global.c
	$(CC) $(CFLAGS) $@ $<

kliba.o : ./kliba.asm
	$(ASM) -f elf -o $@ $<

stringAsm.o : ./string.asm
	$(ASM) -f elf -o $@ $<

protect.o : ./protect.c
	$(CC) $(CFLAGS) $@ $<

i8259.o : ./i8259.c
	$(CC) $(CFLAGS) $@ $<

klib.o : ./klib.c
	$(CC) $(CFLAGS) $@ $<
