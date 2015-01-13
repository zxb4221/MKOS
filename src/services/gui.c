#include "../include/type.h"
#include "../include/const.h"
#include "../include/fs.h"
#include "../include/protect.h"
#include "../include/proc.h"
#include "../include/tty.h"
#include "../include/console.h"
#include "../include/global.h"
#include "../include/string.h"
#include "../include/proto.h"

#define VIDEO_PADDRESS	0x805C			//此处四字节存放显存起始地址

static void gui_init();
void writeVRAM(unsigned char* vram, u32 pos, int r, int g, int b);
void printChar(unsigned char* vram, int xsize, char c, int x, int y);
void printString(unsigned char* vram, char *string, int size, int xsize, int x, int y);

void* lpASC16 = 0;
unsigned char* VIDEO_VRAM = 0;

void task_gui()
{
	MESSAGE msg;

	gui_init();
	printf("gui tssk run!\n");
	printString(VIDEO_VRAM,"zxb",3,800,0,0);
	while (1)
	{
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		int reply = 1;
		int msgtype = msg.type;

		switch (msgtype)
		{
		case FREE:
			//do_free(&msg);
			reply = 1;
			break;
		default:
			dump_msg("MM::unknown msg", &msg);
			assert(0);
			break;
		}

		if (reply)
		{
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
		}
	}

}
static void gui_init()
{
	VIDEO_VRAM = (unsigned char*)(*((unsigned int*)VIDEO_PADDRESS));

	MESSAGE msg;
	reset_msg(&msg);
	msg.type = OPEN;
	msg.u.m2.m2p1 = "ASC16";
	send_recv(BOTH, TASK_FS, &msg);
	u32 fileSize = msg.u.m3.m3i1;
	int flip = msg.u.m3.m3i2;


	printf("file %s size:0x%x\n", "ASC16",fileSize);

	lpASC16 = (void*)kmalloc(fileSize);
	reset_msg(&msg);
	msg.type = READ;
	msg.u.m3.m3i1 = fileSize;
	msg.u.m3.m3p1 = (void*)lpASC16;
	send_recv(BOTH, TASK_FS, &msg);

	reset_msg(&msg);
	msg.type = CLOSE;
	msg.u.m3.m3i1 = flip;
	send_recv(BOTH, TASK_FS, &msg);

}


void writeVRAM(unsigned char* vram, u32 pos, int r, int g, int b)
{
	unsigned char* p = (unsigned char*)vram;
	p += pos*3;
	*p = r;
	*(p + 1) = g;
	*(p + 2) = b;
}
void printChar(unsigned char* vram, int xsize, char c, int x, int y)
{
  char d;
  int i=0;
  char *pos = (char *)((u32)lpASC16 + (u32)c*16);
  for (i=0;i<16;++i) {
     int p = ((y + i)*xsize + x);
     d = *(pos + i);
     if (d & 0x80) { writeVRAM(vram,p, 0x00, 0xFF, 0x0);}
     if (d & 0x40) { writeVRAM(vram,p + 1, 0xFF, 0xFF, 0x0);}
     if (d & 0x20) { writeVRAM(vram,p + 2, 0xFF, 0xFF, 0x0);}
     if (d & 0x10) { writeVRAM(vram,p + 3, 0xFF, 0xFF, 0x0);}
     if (d & 0x08) { writeVRAM(vram,p + 4, 0xFF, 0xFF, 0x0);}
     if (d & 0x04) { writeVRAM(vram,p + 5, 0xFF, 0xFF, 0x0);}
     if (d & 0x02) { writeVRAM(vram,p + 6, 0xFF, 0xFF, 0x0);}
     if (d & 0x01) { writeVRAM(vram,p + 7, 0xFF, 0xFF, 0x0);}
  }
}
void printString(unsigned char* vram, char *string, int size, int xsize, int x, int y)
{
   int i=0;

   for (i=0;i<size;++i) {
      char c = *(string + i);
      int p = x + i*8;
      int yOffset = p/xsize;
      printChar(vram,xsize,c, p, y+(yOffset*16));
   }
}
