#include "../include/const.h"
#include "../include/type.h"
#include "../include/fs.h"
#include "../include/protect.h"
#include "../include/proc.h"
#include "../include/tty.h"
#include "../include/console.h"
#include "../include/global.h"
#include "../include/string.h"
#include "../include/proto.h"

void setClientBackColor(struct SHEET* sheet,int r, int g, int b);
void drawChar(struct SHEET* sheet, u8 c, int charIndex, int r, int g, int b);
void DrawChar(u32* vram, u32 xsize, u32 ySize, u32 x, u32 y, u8 c, u32 r, u32 g, u32 b);

void init_tty(TTY* p_tty);
void tty_do_read(TTY* p_tty);
MSG getMsg();
char szShellStack[STACK_SIZE_DEFAULT];
void Shell()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = KMALLOC;
	msg.u.m3.m3i1 = 300 * 300 * 4;
	send_recv(BOTH, TASK_MM, &msg);
	u32* windowBuffer = (u32*) msg.u.m3.m3p1;

	reset_msg(&msg);
	msg.type = KMALLOC;
	msg.u.m3.m3i1 = 4000;
	send_recv(BOTH, TASK_MM, &msg);
	u8* charBuffer = (u8*) msg.u.m3.m3p1;
	memset(charBuffer,0,4000);
	int charCount = 0;

	reset_msg(&msg);
	msg.type = CREATEWINDOW;
	msg.u.m3.m3i1 = 200;
	msg.u.m3.m3i2 = 200;
	msg.u.m3.m3i3 = 300;
	msg.u.m3.m3i4 = 300;
	msg.u.m3.m3p1 = (void*) "Shell";
	msg.u.m3.m3p2 = windowBuffer;
	send_recv(BOTH, TASK_GUI, &msg);

	struct SHEET *sht = msg.u.m2.m2p1;
	setClientBackColor(sht, 0, 0, 0);
	reset_msg(&msg);
	msg.type = UPDATEWINDOW;
	msg.u.m3.m3p1 = (void*) sht;
	send_recv(SEND, TASK_GUI, &msg);

	while (1)
	{
		MSG msg1 = getMsg();
		switch (msg1.type)
		{
		case MSG_CHAR:
		{
			char ch = (char)msg1.param1;
			charBuffer[charCount] = ch;
			drawChar(sht, ch, charCount,0xFF, 0xFF, 0xFF);
			charCount++;

			reset_msg(&msg);
			msg.type = UPDATEWINDOW;
			msg.u.m3.m3p1 = (void*) sht;
			send_recv(SEND, TASK_GUI, &msg);
			break;
		}
		default:
			break;
		}
	}

}

MSG getMsg()
{
	struct proc* p = proc_table[8];
	PROC_MSG_LIST* pml = p->pl;
	while(1)
	{
		if (pml->count) {
			MSG msg = *(pml->p_tail);
			pml->p_tail++;
			if (pml->p_tail == pml->data + PROC_MSG_MAX) {
				pml->p_tail = pml->data;
			}
			pml->count--;
			return msg;
		}
	}
}


void drawChar(struct SHEET* sheet, u8 c, int charIndex, int r, int g, int b)
{
	int y = charIndex/36;
	int x = charIndex%36;
	DrawChar(sheet->buf,sheet->bxsize,sheet->bysize,2 + x*8, 32 + y*16, c, r, g, b);
}
void setClientBackColor(struct SHEET* sheet,int r, int g, int b)
{
	int i,j;
	for(i = 32; i < sheet->bysize - 2; ++i)
	{
		for(j = 2; j < sheet->bxsize-2; ++j)
		{
			sheet->buf[(i * sheet->bxsize + j)] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
		}
	}
}
