
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
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
#include "../include/keyboard.h"
#include "../include/proto.h"

//#define TTY_FIRST	(tty_table)
//#define TTY_END		(tty_table + NR_CONSOLES)
PUBLIC	TTY		tty;
PUBLIC	CONSOLE		console;
void flush(CONSOLE* p_con);

void putCharMsgToProc(struct proc* p, u8 ch);

extern unsigned char* VIDEO_VRAM;
void printStringXXX(unsigned char* vram, char *string, int size, int xsize, int x, int y) ;
void init_tty(TTY* p_tty);
void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);

PRIVATE char szTTYBuffer[256] = {0};
PRIVATE int	nTTYBufferIndex = 0;

PUBLIC u32 in_processEx(u32 key);
/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	MESSAGE msg;
	TTY* p_tty;
	int src;
	int f_WaitSend = 0;
	int i = 0;

	p_tty = &tty;
	init_tty(p_tty);
	flush(&console);

	while (1)
	{
		send_recv(RECEIVE, ANY, &msg);
		/*
		tty_do_read(p_tty);
		printStringXXX(VIDEO_VRAM,"T",1,800,0,500);
		if (p_tty->inbuf_count)
		{

			if (*(p_tty->p_inbuf_tail) == '\n')
			{
				tty_do_write(p_tty);
				szTTYBuffer[nTTYBufferIndex] = 0;
				nTTYBufferIndex = 0;

				if (f_WaitSend)
				{
					msg.type = TTY_ENTER;
					msg.u.m2.m2p1 = szTTYBuffer;
					send_recv(SEND, src, &msg);
					f_WaitSend = 0;

				}
			}
			else
			{
				char ch = *(p_tty->p_inbuf_tail);
				tty_do_write(p_tty);
				szTTYBuffer[nTTYBufferIndex++] = ch;

				struct proc* pShell = proc_table[8];
				putCharMsgToProc(pShell,ch);
				printStringXXX(VIDEO_VRAM,"t",1,800,0,550);
			}
		}

		if (f_WaitSend == 0)
		{
			send_recv(RECEIVE, ANY, &msg);
			src = msg.source;
			f_WaitSend = 1;
		}
		*/

	}
}

void putCharMsgToProc(struct proc* p, u8 ch)
{
	PROC_MSG_LIST* pml = p->pl;
	if (pml->count < PROC_MSG_MAX) {
			MSG msg;
			msg.type = MSG_CHAR;
			msg.param1 = ch;

			*(pml->p_head) = msg;
			pml->p_head++;
			if (pml->p_head == pml->data + PROC_MSG_MAX) {
				pml->p_head = pml->data;
			}
			pml->count++;

		}

}
/*======================================================================*
			   init_tty
 *======================================================================*/
void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}
PUBLIC u32 in_processEx(u32 key)
{
        if (!(key & FLAG_EXT)) {
        	return key;
        }
        else {
                int raw_code = key & MASK_RAW;
                switch(raw_code) {
                case ENTER:
			return '\n';
			break;
                case BACKSPACE:
			return '\b';
			break;
                case UP:
                	/*
                        if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
				scroll_screen(p_tty->p_console, SCR_DN);
                        }
                        */
			break;
		case DOWN:
			/*
			if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
				scroll_screen(p_tty->p_console, SCR_UP);
			}
			*/
			break;
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:
		case F10:
		case F11:
		case F12:
			/* Alt + F1~F12 */
			if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
				//select_console(raw_code - F1);
			}
			else {
				if (raw_code == F12) {
					disable_int();
					dump_proc(proc_table[4]);
					for(;;);
				}
			}
			break;
                default:
                        break;
                }
        }
        return 0;
}
/*======================================================================*
				in_process
 *======================================================================*/
/*
PUBLIC void in_process(TTY* p_tty, u32 key)
{
        if (!(key & FLAG_EXT)) {
		put_key(p_tty, key);
        }
        else {
                int raw_code = key & MASK_RAW;
                switch(raw_code) {
                case ENTER:
			put_key(p_tty, '\n');
			break;
                case BACKSPACE:
			put_key(p_tty, '\b');
			break;
                case UP:
                        if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
				scroll_screen(p_tty->p_console, SCR_DN);
                        }
			break;
		case DOWN:
			if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
				scroll_screen(p_tty->p_console, SCR_UP);
			}
			break;
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:
		case F10:
		case F11:
		case F12:
			// Alt + F1~F12
			if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
				//select_console(raw_code - F1);
			}
			else {
				if (raw_code == F12) {
					disable_int();
					dump_proc(proc_table[4]);
					for(;;);
				}
			}
			break;
                default:
                        break;
                }
        }
}
*/

/*======================================================================*
			      put_key
*======================================================================*/
/*
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;

	}
}
*/

/*======================================================================*
			      tty_do_read
 *======================================================================*/

/*
void tty_do_read(TTY* p_tty)
{

	keyboard_read(p_tty);
}
*/

/*======================================================================*
			      tty_do_write
 *======================================================================*/
/*
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}
*/
/*======================================================================*
                              tty_write
*======================================================================*/
/*
PUBLIC void tty_write(TTY* p_tty, char* buf, int len)
{
        char* p = buf;
        int i = len;

        while (i) {
                out_char(p_tty->p_console, *p++);
                i--;
        }
}
*/
void* va2py(struct proc* p, void* va)
{
	u32 vAddr = (u32)va;
	u32 dirIndex = vAddr >> 22;
	u32 pageIndex = (vAddr >> 12) & 0x3FF;	//get middle 10bits
	u32 offset = vAddr & 0xFFF;	//low 12 bits

	u32 pageTable = (*(u32*)(p->pageDirBase + 4*dirIndex)) & 0xFFFFF000;
	u32 page = (*(u32*)(pageTable + 4*pageIndex)) & 0xFFFFF000;
	return (void*)(page + offset);

}

void SetKernelCR3();
/*======================================================================*
                              sys_printx
*======================================================================*/
PUBLIC int sys_printx(int _unused1, int _unused2, char* s, struct proc* p_proc)
{
	const char * p;
	char ch;

	char reenter_err[] = "? k_reenter is incorrect for unknown reason";




	reenter_err[0] = MAG_CH_PANIC;

	/**
	 * @note Code in both Ring 0 and Ring 1~3 may invoke printx().
	 * If this happens in Ring 0, no linear-physical address mapping
	 * is needed.
	 *
	 * @attention The value of `k_reenter' is tricky here. When
	 *   -# printx() is called in Ring 0
	 *      - k_reenter > 0. When code in Ring 0 calls printx(),
	 *        an `interrupt re-enter' will occur (printx() generates
	 *        a software interrupt). Thus `k_reenter' will be increased
	 *        by `kernel.asm::save' and be greater than 0.
	 *   -# printx() is called in Ring 1~3
	 *      - k_reenter == 0.
	 */
	if (k_reenter == 0)  /* printx() called in Ring<1~3> */
	{
		//p = va2la(proc2pid(p_proc), s);
		//zxb
		p = va2py(p_proc,s);
	}
	else if (k_reenter > 0) /* printx() called in Ring<0> */
		p = s;
	else	/* this should NOT happen */
		p = reenter_err;


	/**
	 * @note if assertion fails in any TASK, the system will be halted;
	 * if it fails in a USER PROC, it'll return like any normal syscall
	 * does.
	 */
	if ((*p == MAG_CH_PANIC) ||
	    (*p == MAG_CH_ASSERT && proc2pid(p_proc_ready) < NR_TASKS)) {
		disable_int();
		char * v = (char*)V_MEM_BASE;
		const char * q = p + 1; /* +1: skip the magic char */

		while (v < (char*)(V_MEM_BASE + V_MEM_SIZE)) {
			*v++ = *q++;
			*v++ = RED_CHAR;
			if (!*q) {
				while (((int)v - V_MEM_BASE) % (SCR_WIDTH * 16)) {
					/* *v++ = ' '; */
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1;
			}
		}

		__asm__ __volatile__("hlt");
	}

	while ((ch = *p++) != 0) {
		if (ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
			continue; /* skip the magic char */

		out_char(tty.p_console, ch);
	}

	return 0;
}

