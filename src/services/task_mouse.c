/*
 * task_mouse.c
 *
 *  Created on: 2015年2月13日
 *      Author: zxb
 */
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
#include "../include/keyboard.h"

struct MOUSE_DEC{
	unsigned char buf[3];
	unsigned char phase;
	int x,y,btn;
};
extern struct SHEET* sht_mouse;
extern struct SHEET *sht_back;
extern struct SHEET *sht_win;
extern KB_INPUT	mouse_in;
extern unsigned char* VIDEO_VRAM;
void sheet_slideEx(struct SHEET *sht, int vx0, int vy0);
void sheet_refreshEx(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void printStringXXX(unsigned char* vram, char *string, int size, int xsize, int x, int y) ;
static u8 get_byte_from_mousebuf();
int mouse_decode(struct MOUSE_DEC* mdec, unsigned char dat);
void task_mouse()
{
	int curSorX = 400;
		int curSorY = 400;
	char buffer[28];
	struct MOUSE_DEC mdec = {0};
	MESSAGE msg;
	reset_msg(&msg);
		msg.type = GUILOG;
		msg.u.m2.m2p1 = "LOG";

	send_recv(SEND, TASK_GUI, &msg);
	while (1)
	{
		if (mouse_in.count > 0)
		{
			u8 data = get_byte_from_mousebuf();
			//enable_int();

			if (mouse_decode(&mdec, data) != 0)
			{
				//io_cli();
				/*
				if(mdec.x > 100 || mdec.y > 100 || mdec.x < -100 || mdec.y < -100)
					continue;
				*/
				sprintf(buffer, "[lcr %d,%d]", mdec.x, mdec.y);
				if ((mdec.btn & 0x01) != 0)
					buffer[1] = 'L';
				if ((mdec.btn & 0x02) != 0)
					buffer[3] = 'R';
				if ((mdec.btn & 0x04) != 0)
					buffer[2] = 'C';

				curSorX += mdec.x;
				curSorY += mdec.y;
				if (curSorX < 0)
					curSorX = 0;
				if (curSorY < 0)
					curSorY = 0;
				if (curSorX > 800 - 2)
					curSorX = 800 - 2;
				if (curSorY > 600 - 4)
					curSorY = 600 - 4;

				boxfill(sht_back->buf, 800, 0, 0, 800, 16, 0xFF,0x0,0x0);
				DrawString(sht_back->buf, 800, 600, 0, 0, buffer, 0xFF, 0xFF,0xFF);
				//printStringXXX(VIDEO_VRAM,buffer,strlen(buffer),800,5,5);


				sheet_refreshEx(sht_back, 0, 0, 800 , 16);
				//clear_mouse_cursor8(curSorX,curSorY);
				sheet_slideEx(sht_mouse, curSorX, curSorY);
				//sheet_slide(sht_mouse, curSorX, curSorY);


				 if((mdec.btn & 0x01) != 0)
					 sheet_slideEx(sht_win, curSorX-80, curSorY-8);


			}

		}
		else
		{
			reset_msg(&msg);
			msg.type = GUILOG;
			msg.u.m2.m2p1 = "E";

			//					send_recv(SEND, TASK_GUI, &msg);

		}

	}
}
static u8 get_byte_from_mousebuf()       /* 从键盘缓冲区中读取下一个字节 */
{
        u8 scan_code;

        while (mouse_in.count <= 0) {}   /* 等待下一个字节到来 */

        disable_int();
        scan_code = *(mouse_in.p_tail);
        mouse_in.p_tail++;
        if (mouse_in.p_tail == mouse_in.buf + KB_IN_BYTES) {
        	mouse_in.p_tail = mouse_in.buf;
        }
        mouse_in.count--;
        enable_int();

	return scan_code;
}
int mouse_decode(struct MOUSE_DEC* mdec, unsigned char dat)
{
	if (mdec->phase == 0)
	{
		if (dat == 0xfa)
		{
			mdec->phase = 1;
		}
		return 0;
	}
	else if (mdec->phase == 1)
	{
		if((dat & 0xc8) == 0x08){
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}

		return 0;
	}
	else if (mdec->phase == 2)
	{
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	else if (mdec->phase == 3)
	{
		mdec->buf[2] = dat;
		mdec->phase = 1;

		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];

		if((mdec->buf[0] & 0x10) != 0){
			mdec->x |= 0xffffff00;
		}
		if((mdec->buf[0] & 0x20) != 0){
			mdec->y |= 0xffffff00;
		}
		mdec->y = -mdec->y;
		return 1;

	}
	return -1;
}
