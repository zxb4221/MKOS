/*
 * mouse.c
 *
 *  Created on: 2015年2月13日
 *      Author: zxb
 */


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            keyboard.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "../include/const.h"
#include "../include/type.h"
#include "../include/fs.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/tty.h"
#include "../include/console.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/keyboard.h"

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

extern unsigned char* VIDEO_VRAM;
void printStringXXX(unsigned char* vram, char *string, int size, int xsize, int x, int y) ;
void wait_KBC_sendready(void);
KB_INPUT	mouse_in = {0};
PUBLIC void mouse_handler(int irq)
{
	char buffer[12] = {0};
	unsigned char data;
		io_out8(0x00a0, 0x64);	/* IRQ-12庴晅姰椆傪PIC1偵捠抦 */
		io_out8(0x0020, 0x62);	/* IRQ-02庴晅姰椆傪PIC0偵捠抦 */
		data = io_in8(0x0060);

		if (mouse_in.count < KB_IN_BYTES) {
				*(mouse_in.p_head) = data;
				mouse_in.p_head++;
				if (mouse_in.p_head == mouse_in.buf + KB_IN_BYTES) {
					mouse_in.p_head = mouse_in.buf;
				}
				mouse_in.count++;
			}
		printStringXXX(VIDEO_VRAM,"M",1,800,0,200);
}

PUBLIC void init_mouse()
{

	wait_KBC_sendready();
		io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
		wait_KBC_sendready();
		io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);

		//set_leds_mouse();
		//boxfill(0,580,200,600);
		put_irq_handler(0xc, mouse_handler);/*设定mouse中断处理程序*/
		enable_irq(0xc);
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
