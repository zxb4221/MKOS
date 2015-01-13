
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "../include/type.h"
#include "../include/const.h"
#include "../include/fs.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/tty.h"
#include "../include/console.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/elf32.h"


PUBLIC void StartShell();

void *kmalloc(unsigned int len);

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	struct task* p_task;
	struct proc* p_proc;
	char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	u8 privilege;
	u8 rpl;
	int eflags;
	int i;
	int prio;
	for (i = 0; i < NR_TASKS; i++)
	{
		p_proc = proc_table[i] = &proc_table_task[i];

		p_task = task_table + i;
		privilege = PRIVILEGE_TASK;
		rpl = RPL_TASK;
		eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
		prio = 1;

		strcpy(p_proc->name, p_task->name); /* name of the process */
		p_proc->p_parent = NO_TASK;
		//p_proc->pid = i;			/* pid */

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
				sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
				sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;

		p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32) p_task->initial_eip;
		p_proc->regs.esp = (u32) p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty = 0;

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		p_proc->ticks = p_proc->priority = prio;

		p_proc->pageDirBase = PAGE_DIR_BASE;

		p_task_stack -= p_task->stacksize;
		p_task++;
		selector_ldt += 1 << 3;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table[0];

	init_clock();

	init_keyboard();

	disp_str("-----\"kernel_main\" finished-----\n");
	restart();

	while (1)
	{
	}
}

/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}

PUBLIC int wait(int * status)
{
    MESSAGE msg;
    msg.type = WAIT;
    send_recv(BOTH, TASK_MM, &msg);
    *status = msg.STATUS;
    return (msg.PID == NO_TASK ? -1 : msg.PID);
}






int GetEmptyProcessID()
{
	int i;
	for (i = 0; i < NR_ALL_PROCS; ++i)
	{
		if (0 == proc_table[i])
			return i;
	}
	panic("Can't find a empty Process ID\n");
	return 0;
}

void* libkmalloc(unsigned int size)
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = KMALLOC;
	msg.u.m3.m3i1 = size;
	send_recv(BOTH, TASK_MM, &msg);
	return msg.u.m3.m3p1;
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

