ASM	= nasm
LD = ld
CC = gcc
CFLAGS = -g -c -m32  -fno-builtin
TARGET = ECHO
OBJS = echo.o \
	   ../lib/start.o \
	   ../lib/exit.o \
	   ../lib/misc.o \
	   ../lib/syscall.o \
	   ../lib/string.o \
	   ../lib/printf.o \
	   ../lib/vsprintf.o
	   
all:$(TARGET)

$(TARGET) : $(OBJS)
	$(LD) -Ttext 0x2800000 -m elf_i386 -o $(TARGET) $(OBJS)

echo.o : echo.c
	$(CC) $(CFLAGS) -o $@ $<