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

extern char szShellStack[STACK_SIZE_DEFAULT];
void Shell();
int GetEmptyProcessID();
static void StartShell();

void task_init()
{
    StartShell();

    while(1)
    {
        //printf("s");
    }
}
u32 do_kmalloc_inner(u32 size);
static void StartShell()
{
	u8 privilege;
	u8 rpl;
	int eflags;
	int prio;
	struct proc* p_proc;
	int pid;
	int i;

	pid = GetEmptyProcessID();
	if (pid == -1)
		panic("getNewProcId failed,pid:%d\n", pid);

	privilege = PRIVILEGE_TASK;
	rpl = RPL_TASK;
	eflags = 0x202; /* IF=1, bit 2 is always 1 */
	prio = 1;

	p_proc = proc_table[pid] = (struct proc*) do_kmalloc_inner(sizeof(struct proc));

	memset(p_proc, 0, sizeof(struct proc));
	p_proc->p_flags = 1;

	p_proc->ldt_sel = SELECTOR_LDT_FIRST + (pid << 3);
	init_desc(&gdt[INDEX_LDT_FIRST + pid],
			makelinear(SELECTOR_KERNEL_DS, p_proc->ldts),
			LDT_SIZE * sizeof(struct descriptor) - 1,
			DA_LDT);

	strcpy(p_proc->name, "SHELL"); /* name of the process */
	p_proc->p_parent = NO_TASK;
	//p_proc->pid = i;			/* pid */

	//p_proc->ldt_sel = selector_ldt;
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

	p_proc->regs.eip = (u32) Shell;
	p_proc->regs.esp = (u32) (szShellStack + STACK_SIZE_DEFAULT);
	p_proc->regs.eflags = eflags;

	p_proc->nr_tty = 0;

	p_proc->p_msg = 0;
	p_proc->p_recvfrom = NO_TASK;
	p_proc->p_sendto = NO_TASK;
	p_proc->has_int_msg = 0;
	p_proc->q_sending = 0;
	p_proc->next_sending = 0;

	p_proc->ticks = p_proc->priority = prio;

	p_proc->pageDirBase = PAGE_DIR_BASE;
	p_proc->p_flags = 0;
}
