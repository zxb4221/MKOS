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

#define VIDEO_PADDRESS	0x805C			//此处四字节存放显存起始地址


/* sheet.c */




struct MOUSE_DEC{
	unsigned char buf[3];
	unsigned char phase;
	int x,y,btn;
};

#define SHEET_USE		1
extern unsigned char* VIDEO_VRAM;



typedef struct
{
	//unsigned short    bfType;
	unsigned long    bfSize;
	unsigned short    bfReserved1;
	unsigned short    bfReserved2;
	unsigned long    bfOffBits;
} ClBitMapFileHeader;

typedef struct
{
	unsigned long  biSize;
	long   biWidth;
	long   biHeight;
	unsigned short   biPlanes;
	unsigned short   biBitCount;
	unsigned long  biCompression;
	unsigned long  biSizeImage;
	long   biXPelsPerMeter;
	long   biYPelsPerMeter;
	unsigned long   biClrUsed;
	unsigned long   biClrImportant;
} ClBitMapInfoHeader;

typedef struct
{
	unsigned char rgbBlue; //该颜色的蓝色分量
	unsigned char rgbGreen; //该颜色的绿色分量
	unsigned char rgbRed; //该颜色的红色分量
	unsigned char rgbReserved; //保留值
} ClRgbQuad;


void sheet_refreshEx(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void printStringXXX(unsigned char* vram, char *string, int size, int xsize, int x, int y) ;
static void gui_init();

void DrawPoint(u32* vram, u32 xSize, u32 ySize, u32 x, u32 y, u32 r, u32 g, u32 b);
void DrawChar(u32* vram, u32 xsize, u32 ySize, u32 x, u32 y, u8 c, u32 r, u32 g, u32 b);
void DrawString(u32* vram, u32 xSize, u32 ySize, u32 x, u32 y, u8* string, u32 r, u32 g, u32 b);


void sheet_refreshsubEx(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0,int h1);
void sheet_slideEx(struct SHEET *sht, int vx0, int vy0);


void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);
void init_mouse_cursor8(u32* vram);
void make_window8(u32 *buf, int xsize, int ysize, char *title);

struct SHTCTL *shtctl_init(unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
struct SHEET *sheet_allocEx(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, u32 *buf, int xsize, int ysize);
void sheet_updown(struct SHEET *sht, int height);
void sheet_updownEx(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

void* do_createWindow(MESSAGE* lpMsg);
void do_updateWindow(MESSAGE* lpMsg);
void updateWindow();

void* lpASC16 = 0;
unsigned char* lpBK = 0;
unsigned char* VIDEO_VRAM = 0;
struct SHTCTL* shtctl = 0;

struct SHEET *sht_back, *sht_mouse, *sht_win;

void task_gui()
{
	MESSAGE msg;

	gui_init();
	printf("gui task run!\n");
	while (1)
	{
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		int reply = 1;
		int msgtype = msg.type;

		switch (msgtype)
		{
		case CREATEWINDOW:
			do_createWindow(&msg);
			reply = 1;
			break;
		case UPDATEWINDOW:
			do_updateWindow(&msg);
			reply = 0;
			break;
		case GUILOG:
			//DrawString(sht_back->buf,sht_back->bxsize,sht_back->bysize,5,5,(u8*)msg.u.m2.m2p1, 0xFF,0xFF,0xFF);
			//updateWindow();
			reply = 0;
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
void updateWindow()
{
	sheet_refreshsubEx(shtctl,0,0,800,600,0,3);
}
void do_updateWindow(MESSAGE* lpMsg)
{
	struct SHEET* lpSheet = lpMsg->u.m3.m3p1;
	//sheet_slideEx(lpSheet, 200, 200);
	sheet_refreshEx(lpSheet,0,0,lpSheet->bxsize,lpSheet->bysize);
}
void* do_createWindow(MESSAGE* lpMsg)//u32 x, u32 y, u32 width, u32 height,u8* title,u32* windowBuffer)
{
	u32 x = lpMsg->u.m3.m3i1;
	u32 y = lpMsg->u.m3.m3i2;
	u32 width = lpMsg->u.m3.m3i3;
	u32 height = lpMsg->u.m3.m3i4;
	u8* title = (u8*)lpMsg->u.m3.m3p1;
	u32* windowBuffer = (u32*)lpMsg->u.m3.m3p2;
	struct SHEET* lpSheetWindow = sheet_alloc(shtctl);
	sheet_setbuf(lpSheetWindow, windowBuffer, width, height);
	make_window8(windowBuffer,width, height, title);

	//sheet_refreshEx(lpSheetWindow,0,0,lpSheetWindow->bxsize,lpSheetWindow->bysize);
	sheet_slideEx(lpSheetWindow, x, y);
	//sheet_updownEx(sht_mouse,3);
	sheet_updownEx(lpSheetWindow,2);
	sheet_updownEx(sht_mouse,3);
	//sheet_refreshEx(shtctl,0,0,800,600,0,3);
	lpMsg->u.m2.m2p1 = lpSheetWindow;
	return lpSheetWindow;
}
void boxfill(u32* vram ,int xsize, int x0, int y0, int x1,int y1,int r, int g, int b)
{
	int x, y;
		for (y = y0; y <= y1; y++) {
			for (x = x0; x <= x1; x++)
			{
				vram[(y * xsize + x)] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
			}
		}
}
void make_window8(u32 *buf, int xsize, int ysize, char *title)
{
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c;
	boxfill((u32*)buf, xsize, 0,0,xsize,ysize,187,187, 187);
	boxfill((u32*)buf, xsize, 2,2,xsize-2,30,0,0, 187);


	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				buf[((5 + y) * xsize + (xsize - 21 + x))] = 0xFF;
			} else if (c == '$') {
				buf[((5 + y) * xsize + (xsize - 21 + x))] = 0xFF | 0xFF00 | 0xFF0000 | 0xFF000000;
			} else if (c == 'Q') {
				buf[((5 + y) * xsize + (xsize - 21 + x))] = 0xFF | 0xBB00 | 0xBB0000 | 0xBB000000;

			} else {
				//c = COL8_FFFFFF;
			}


		}

		DrawString(buf,xsize,ysize,5,5,title, 0xFF,0xFF,0xFF);
		//printString(buf,title, strlen(title), xsize,5, 5);
	}

	return;
}
void init_mouse_cursor8(u32* vram)
{
	static char cursor[16][16] = {
		"*...............",
		"**..............",
		"*O*.............",
		"*OO*............",
		"*OOO*...........",
		"*OOOO*..........",
		"*OOOOO*.........",
		"*OOOOOO*........",
		"*OOOOOOO*.......",
		"*OOOO****.......",
		"*OO*O*..........",
		"*O*.*O*.........",
		"**..*O*.........",
		"*....*O*........",
		".....*O*........",
		"......*........."
	};
	int x, y;
	memset((void*)vram, 0, 16*16*4);
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
				 DrawPoint(vram,16,16,x,y,0xF, 0xF, 0xF);
			}
			else if (cursor[y][x] == 'O') {
				 DrawPoint(vram,16,16,x,y,0xFF, 0xFF, 0xFF);
			}
		}
	}
}

static void gui_init()
{
	VIDEO_VRAM = (unsigned char*) (*((unsigned int*) VIDEO_PADDRESS));

	MESSAGE msg;
	reset_msg(&msg);
	msg.type = OPEN;
	msg.u.m2.m2p1 = "ASC16";
	send_recv(BOTH, TASK_FS, &msg);
	u32 fileSize = msg.u.m3.m3i1;
	int flip = msg.u.m3.m3i2;

	printf("file %s size:0x%x\n", "ASC16", fileSize);

	lpASC16 = (void*) kmalloc(fileSize);
	reset_msg(&msg);
	msg.type = READ;
	msg.u.m3.m3i1 = fileSize;
	msg.u.m3.m3p1 = (void*) lpASC16;
	msg.u.m3.m3i3 = flip;
	send_recv(BOTH, TASK_FS, &msg);

	reset_msg(&msg);
	msg.type = CLOSE;
	msg.u.m3.m3i1 = flip;
	send_recv(BOTH, TASK_FS, &msg);

	VIDEO_VRAM = (unsigned char*) (*((unsigned int*) VIDEO_PADDRESS));
	printStringXXX(VIDEO_VRAM,"V",1,800,100,100);
	int mx;
	int my;
	shtctl = shtctl_init((unsigned char *) VIDEO_VRAM, 800, 600);



	u32* lpBufferBack = (u32*)kmalloc(800*600*4);
	u32* lpBufferWin = (u32*)kmalloc(160*68*4);
	u32* lpBufferMouse = (u32*)kmalloc(16*16*4);





	sht_back = sheet_alloc(shtctl);
	sheet_setbuf(sht_back, (u32*)lpBufferBack, 800, 600);
	boxfill(lpBufferBack,800,0, 0, 800, 600,58,110,165);
	sheet_updownEx(sht_back,0);
	//sheet_refreshEx(sht_back,0,0,800,600);

	sht_win = sheet_alloc(shtctl);
	sheet_setbuf(sht_win, lpBufferWin, 160, 68);
	make_window8(lpBufferWin, 160, 68, "window");
	sheet_slideEx(sht_win, 80, 72);
	sheet_updownEx(sht_win,1);


	sht_mouse = sheet_alloc(shtctl);
		sheet_setbuf(sht_mouse, lpBufferMouse, 16, 16);
		init_mouse_cursor8(lpBufferMouse);
		mx = (800 - 16) / 2;
		my = (600 - 28 - 16) / 2;
		sheet_slideEx( sht_mouse, 100, 0);
		sheet_updownEx(sht_mouse,2);
		/*
*/

	//sheet_updown(sht_win, 1);
	//sheet_refresh(sht_win, 0, 0, 160, 68);


	//sheet_updown(sht_mouse, 2);
	//sheet_refresh(sht_mouse, 0, 0, 16, 16);

	//u32* windowBuffer = (u32*)kmalloc(300*300*4);
	//do_createWindow(200,200,300,300,"Caption",windowBuffer);



	//sheet_refreshsubEx(shtctl,0,0,800,600,0,3);
	//init_screen8(VIDEO_VRAM, 800, 600);
	/*
	 boxfill(buf_back,800,0, 0, 400, 300,255,0,0);
	 boxfill(buf_back,800,400, 0, 800, 300,0,255,0);
	 boxfill(buf_back,800,0,300, 400, 600 ,0,0,255);
	 boxfill(buf_back,800,400,300, 800, 600,255,255,0);
	 */

	//

	//make_window8(buf_win, 160, 68, "window");
	//printString(buf_win,"Content", 8, 160,5, 35);

	//putfonts8_asc(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
	//putfonts8_asc(buf_win, 160, 24, 44, COL8_000000, "  Haribote-OS!");

	//sheet_slide(sht_win, 80, 72);
	//sheet_updown(sht_win, 0);


	//sheet_refresh(sht_back, 0, 0, 800, 600);
	//sheet_refresh(sht_win, 0, 0, 800, 600);
	//sheet_refresh(sht_mouse, 0, 0, 800, 600);

	/*
	 reset_msg(&msg);
	 msg.type = OPEN;
	 msg.u.m2.m2p1 = "BK";
	 send_recv(BOTH, TASK_FS, &msg);
	 fileSize = msg.u.m3.m3i1;
	 flip = msg.u.m3.m3i2;

	 char buffer[28] =
	 { 0 };
	 sprintf(buffer, "%s,%d", "BK.bmp", fileSize);
	 printString(VIDEO_VRAM, buffer, strlen(buffer), 800, 0, 20);

	 lpBK = (unsigned char*) kmalloc(fileSize);

	 reset_msg(&msg);
	 msg.type = READ;
	 msg.u.m3.m3i1 = fileSize;
	 msg.u.m3.m3p1 = (void*) lpBK;
	 msg.u.m3.m3i3 = flip;
	 send_recv(BOTH, TASK_FS, &msg);

	 reset_msg(&msg);
	 msg.type = CLOSE;
	 msg.u.m3.m3i1 = flip;
	 send_recv(BOTH, TASK_FS, &msg);

	 ClBitMapFileHeader* bmpFileHeader = (ClBitMapFileHeader*) (lpBK + 2);
	 ClBitMapInfoHeader* bmpInfoHeader = (ClBitMapInfoHeader*) (lpBK + 2
	 + sizeof(ClBitMapFileHeader));
	 sprintf(buffer, "w:%d,h:%d", bmpInfoHeader->biWidth,
	 bmpInfoHeader->biHeight);
	 printString(VIDEO_VRAM, buffer, strlen(buffer), 800, 0, 40);

	 char* imageData = (lpBK + 2 + sizeof(ClBitMapFileHeader)
	 + sizeof(ClBitMapInfoHeader));
	 int i, j;
	 for (i = 0; i < bmpInfoHeader->biHeight; i++)
	 {
	 for (j = 0; j < bmpInfoHeader->biWidth; j++)
	 {
	 int pos = (j + i * 800);
	 unsigned char b = *(unsigned char*) (imageData
	 + (bmpInfoHeader->biHeight - 1 - i) * 3
	 * bmpInfoHeader->biWidth + j * 3);
	 unsigned char g = *(unsigned char*) (imageData
	 + (bmpInfoHeader->biHeight - 1 - i) * 3
	 * bmpInfoHeader->biWidth + j * 3 + 1);
	 unsigned char r = *(unsigned char*) (imageData
	 + (bmpInfoHeader->biHeight - 1 - i) * 3
	 * bmpInfoHeader->biWidth + j * 3 + 2);
	 writeVRAM(VIDEO_VRAM, pos, r, g, b);
	 }
	 }

	 */
	//DrawString(lpBufferBack, 800, 600, 0, 580, "OK!", 255, 255, 255);
	//sheet_refresh(sht_back, 0, 580, 800, 600);
	//sheet_refresh(sht_mouse, 0, 0, 800, 600);
}



struct SHTCTL *shtctl_init(unsigned char *vram, int xsize, int ysize)
{
	//static struct SHTCTL shtctl;
	//static u32 mapBuffer[800*600];
	int i;
	struct SHTCTL *ctl = (struct SHTCTL *)kmalloc(sizeof(struct SHTCTL));
	u32* mapBuffer = (u32*)kmalloc(800*600*sizeof(u32));

	memset(ctl,0,sizeof(struct SHTCTL));
	memset(mapBuffer,0,800*600*sizeof(u32));

	if (ctl == 0) {
		goto err;
	}
	ctl->map = mapBuffer;

	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;

	ctl->top = -1;
	for (i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0; /* 枹巊梡儅乕僋 */
		ctl->sheets0[i].ctl = ctl;
		ctl->sheets[i] = 0;
	}
err:
	return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
	struct SHEET *sht = (struct SHEET*) kmalloc(sizeof(struct SHEET));
	memset(sht, 0, sizeof(struct SHEET));
	sht->ctl = ctl;
	int i;
	for (i = 0; i < MAX_SHEETS; i++)
	{
		if (!ctl->sheets[i])
		{
			ctl->sheets[i] = sht;
			ctl->sheets[i]->height = -1;
			ctl->sheets[i]->flags = SHEET_USE;
			//ctl->top = i;
			return sht;
		}
	}
	return 0;
}

void sheet_setbuf(struct SHEET *sht, u32* buf, int xsize, int ysize)
{
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	//sht->height = z;
	return;
}
void sheet_refreshsubEx(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0,int h1)
{
	//io_cli();
	int h, i, j;
	u32 *buf, c, *map = ctl->map, sid;
	u8* vram = ctl->vram;
	struct SHEET *sht;

	/* 如果REFRESH的范围超出了画面则修正*/
	if (vx0 < 0)
	{
		vx0 = 0;
	}
	if (vy0 < 0)
	{
		vy0 = 0;
	}
	if (vx1 > ctl->xsize)
	{
		vx1 = ctl->xsize;
	}
	if (vy1 > ctl->ysize)
	{
		vy1 = ctl->ysize;
	}

	for (h = h0; h <= h1; h++)
	{

		sht = ctl->sheets[h];
		if(!sht)
			continue;
		sid = (u32)sht->buf;
		/*
		if (sht->height < 0)
			continue;
		*/

		buf = sht->buf;
		int bx0 = vx0 - sht->vx0;
		int by0 = vy0 - sht->vy0;
		int bx1 = vx1 - sht->vx0;
		int by1 = vy1 - sht->vy0;
		if(bx0 < 0)bx0 = 0;
		if(by0 < 0)by0 = 0;
		if(bx1 > sht->bxsize)bx1 = sht->bxsize;
		if(by1 > sht->bysize)by1 = sht->bysize;

		for (i = by0; i <= by1; ++i)
		{
			for (j = bx0; j < bx1; ++j)
			{
				int vy = sht->vy0 + i;
				int vx = sht->vx0 + j;
				if (map[vy*ctl->xsize + vx] == sid)
				{
					u32 bufferY = vy - sht->vy0;
					u32 BufferX = vx - sht->vx0;
					if(buf[(bufferY * sht->bxsize + BufferX)] & 0xFF)
					{
						vram[(vy * ctl->xsize + vx) * 3] = (buf[(bufferY * sht->bxsize + BufferX)] >> 8) & 0xFF;
						vram[(vy * ctl->xsize + vx) * 3 + 1] = (buf[(bufferY * sht->bxsize + BufferX)] >> 16) & 0xFF;
						vram[(vy * ctl->xsize + vx) * 3 + 2] = (buf[(bufferY * sht->bxsize + BufferX)] >> 24) & 0xFF;
					}
				}
			}
		}
	}
	//io_sti();
}
void sheet_refreshmapEx(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	u32 *buf, sid, *map = ctl->map;
	struct SHEET *sht;
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		sid = (u32)sht->buf;
		buf = sht->buf;
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				map[vy * ctl->xsize + vx] = sid;
			}
		}
	}
	return;
}
/*
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0,int h1)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	u32 *buf, c,  *map = ctl->map, sid;
	u8* vram = ctl->vram;
	struct SHEET *sht;


	if(vx0 < 0) {vx0 = 0;}
	if(vy0 < 0) {vy0 = 0;}
	if(vx1 > ctl->xsize) {vx1 = ctl->xsize;}
	if(vy1 > ctl->ysize) {vy1 = ctl->ysize;}


	for (h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;

		sid = sht-ctl->sheets0;


		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;

				if (map[vy*ctl->xsize + vx] == sid) {
					if((buf[(by * sht->bxsize + bx)] & 0xFF))
					{
						vram[(vy * ctl->xsize + vx)*3] = (buf[(by * sht->bxsize + bx)] >> 8) & 0xFF ;
						vram[(vy * ctl->xsize + vx)*3 + 1] = (buf[(by * sht->bxsize + bx)] >> 16) & 0xFF;
						vram[(vy * ctl->xsize + vx)*3 + 2] = (buf[(by * sht->bxsize + bx)] >> 24) & 0xFF;
					}
				}
			}
		}
	}
	return;
}
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	u32 *buf, sid, *map = ctl->map;
	struct SHEET *sht;
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		sid = sht - ctl->sheets0;
		buf = sht->buf;
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				if (buf[by * sht->bxsize + bx] != sht->col_inv) {
					map[vy * ctl->xsize + vx] = sid;
				}
			}
		}
	}
	return;
}

void sheet_updown(struct SHEET *sht, int height)
{
	struct SHTCTL* ctl = sht->ctl;
	int h, old = sht->height;


	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height;

	if (old > height) {
		if (height >= 0) {

			for (h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);

		} else {
			if (ctl->top > old) {

				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
		}

	} else if (old < height) {
		if (old >= 0) {

			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {

			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++;
		}
		sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
	}
	return;
}

void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
	struct SHTCTL* ctl = sht->ctl;
	if (sht->height >= 0) {
		sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1,sht->height,sht->height);
	}
	return;
}*/


void sheet_updownEx(struct SHEET *sht, int height)
{
	struct SHTCTL* ctl = sht->ctl;
	int h, old = sht->height;


	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height;

	if (old > height) {
		if (height >= 0) {

			for (h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
			sheet_refreshmapEx(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
			sheet_refreshsubEx(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);

		} else {
			if (ctl->top > old) {

				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--;
			sheet_refreshmapEx(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
			sheet_refreshsubEx(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
		}

	} else if (old < height) {
		if (old >= 0) {

			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {

			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++;
		}
		sheet_refreshmapEx(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
		sheet_refreshsubEx(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
	}
	return;
}


//bx0,by0,bx1,by1 是相对于窗口的坐标
void sheet_refreshEx(struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
	struct SHTCTL* ctl = sht->ctl;
	if (sht->height >= 0) { /* 傕偟傕昞帵拞側傜丄怴偟偄壓偠偒偺忣曬偵増偭偰夋柺傪昤偒捈偡 */
		sheet_refreshsubEx(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1,sht->height,sht->height);
	}
	return;
}
void sheet_slideEx(struct SHEET *sht, int vx0, int vy0)
{
	struct SHTCTL* ctl = sht->ctl;
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) {
			sheet_refreshmapEx(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
			sheet_refreshmapEx(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);

			sheet_refreshsubEx(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
			sheet_refreshsubEx(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
		}
}
/*
void sheet_slide(struct SHEET *sht, int vx0, int vy0)
{
	struct SHTCTL* ctl = sht->ctl;
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) {
		sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
		sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);

		sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
		sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
	}
	return;
}*/
void DrawPoint(u32* vram, u32 xSize, u32 ySize, u32 x, u32 y, u32 r, u32 g, u32 b)
{
	vram[(y * xSize + x)] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

void DrawChar(u32* vram, u32 xsize, u32 ySize, u32 x, u32 y, u8 c, u32 r, u32 g, u32 b)
{
	u8 d;
	u32 i = 0;
	u8 *pos = (u8 *) ((u32) lpASC16 + (u32) c * 16);
	for (i = 0; i < 16; ++i)
	{
		int p = ((y + i) * xsize + x);
		d = *(pos + i);
		if (d & 0x80)DrawPoint(vram, xsize, ySize, x, y+i, r,g,b);
		if (d & 0x40)DrawPoint(vram, xsize, ySize, x+1, y+i, r,g,b);
		if (d & 0x20)DrawPoint(vram, xsize, ySize, x+2, y+i, r,g,b);
		if (d & 0x10)DrawPoint(vram, xsize, ySize, x+3, y+i, r,g,b);
		if (d & 0x08)DrawPoint(vram, xsize, ySize, x+4, y+i, r,g,b);
		if (d & 0x04)DrawPoint(vram, xsize, ySize, x+5, y+i, r,g,b);
		if (d & 0x02)DrawPoint(vram, xsize, ySize, x+6, y+i, r,g,b);
		if (d & 0x01)DrawPoint(vram, xsize, ySize, x+7, y+i, r,g,b);
	}
}
void DrawString(u32* vram, u32 xSize, u32 ySize, u32 x, u32 y, u8* string, u32 r, u32 g, u32 b)
{
	u32 i;
	u32 len = strlen(string);
	for (i = 0; i< len; ++i)
	{
	      u8 c = *(string + i);
	      u32 p = x + i*8;
	      u32 yOffset = p/xSize;
	      DrawChar(vram,xSize,ySize,x + i*8, y+(yOffset*16), c, r, g, b);
	   }
}

void writeVRAMXX(unsigned char* vram, u32 pos, int r, int g, int b)
{
	unsigned char* p = (unsigned char*)vram;
	p += pos*3;
	*p = r;
	*(p + 1) = g;
	*(p + 2) = b;
}

void printCharXXX(unsigned char* vram, int xsize, char c, int x, int y) {
  char d;
  int i=0;
  char *pos = (char *)(lpASC16 + (int)c*16);
  for (i=0;i<16;++i) {
     int p = ((y + i)*xsize + x);
     d = *(pos + i);
     if (d & 0x80) { writeVRAMXX(vram,p, 0x00, 0xFF, 0x0);}
     if (d & 0x40) { writeVRAMXX(vram,p + 1, 0xFF, 0xFF, 0x0);}
     if (d & 0x20) { writeVRAMXX(vram,p + 2, 0xFF, 0xFF, 0x0);}
     if (d & 0x10) { writeVRAMXX(vram,p + 3, 0xFF, 0xFF, 0x0);}
     if (d & 0x08) { writeVRAMXX(vram,p + 4, 0xFF, 0xFF, 0x0);}
     if (d & 0x04) { writeVRAMXX(vram,p + 5, 0xFF, 0xFF, 0x0);}
     if (d & 0x02) { writeVRAMXX(vram,p + 6, 0xFF, 0xFF, 0x0);}
     if (d & 0x01) { writeVRAMXX(vram,p + 7, 0xFF, 0xFF, 0x0);}
  }
}
void printStringXXX(unsigned char* vram, char *string, int size, int xsize, int x, int y) {
   int i=0;

   for (i=0;i<size;++i) {
      char c = *(string + i);
      int p = x + i*8;
      int yOffset = p/xsize;
      printCharXXX(vram,xsize,c, p, y+(yOffset*16));
   }
}
