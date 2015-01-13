
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            start.c
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

extern struct proc* p_proc_ready;
PUBLIC void setupPage();
/*======================================================================*
                            cstart
 *======================================================================*/
PUBLIC void cstart()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n-----\"cstart\" begins-----\n");

	/* 将 LOADER 中的 GDT 复制到新的 GDT 中 */
	memcpy(	&gdt,				   /* New GDT */
		(void*)(*((u32*)(&gdt_ptr[2]))),   /* Base  of Old GDT */
		*((u16*)(&gdt_ptr[0])) + 1	   /* Limit of Old GDT */
		);
	/* gdt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sgdt 以及 lgdt 的参数。 */
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(struct descriptor) - 1;
	*p_gdt_base  = (u32)&gdt;

	/* idt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sidt 以及 lidt 的参数。 */
	u16* p_idt_limit = (u16*)(&idt_ptr[0]);
	u32* p_idt_base  = (u32*)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(struct gate) - 1;
	*p_idt_base  = (u32)&idt;

	setupPage();

	init_prot();

	disp_str("-----\"cstart\" finished-----\n");
}

PUBLIC void setupPage()
{
	u32 pageDirBase = PAGE_DIR_BASE;
	u32 pageTbBase =  PAGE_DIR_BASE + 4*1024;
	u32 value;
	u32 i;

	//初始分页目录
	value = (pageTbBase | PG_P | PG_USU | PG_RWW);
	for(i = 0; i < 1024; ++i)
	{
		*((u32*)(pageDirBase + i*4)) = value;
		value += 4096;
	}

	//初始化页表
	value = (PG_P | PG_USU | PG_RWW);
	for(i = 0; i < 1048576; ++i)
	{
		*((u32*)(pageTbBase + i*4)) = value;
		value += 4096;
	}

	setCR3(pageDirBase);

}
void SetKernelCR3()
{
	setCR3(0x500000);
}
void SetProcCR3()
{
	setCR3(p_proc_ready->pageDirBase);
}
