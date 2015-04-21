
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE
#include "../include/const.h"
#include "../include/type.h"

#include "../include/fs.h"
#include "../include/protect.h"
#include "../include/tty.h"
#include "../include/console.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"


PUBLIC	struct proc proc_table_task[NR_ALL_PROCS] = {0};
PUBLIC	struct proc* proc_table[NR_ALL_PROCS] = {0};

PUBLIC	struct task	task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "TTY"},
	{task_sys, STACK_SIZE_SYS, "SYS"},
	{task_hd, STACK_SIZE_HD, "HD"},
	{task_fs, STACK_SIZE_FS, "FS"},
	{task_mm, STACK_SIZE_MM, "MM"},
	{task_gui, STACK_SIZE_GUI, "GUI"},
	{task_init, STACK_SIZE_INIT, "INIT"},
	{task_mouse, STACK_SIZE_MOUSE, "MOUSE"}
	};
/*
PUBLIC	struct task	user_proc_table[NR_PROCS] = {
	{Init, STACK_SIZE_INIT, "INIT"}/*,
	{TestA, STACK_SIZE_TESTA, "TestA"},
	{TestB, STACK_SIZE_TESTB, "TestB"},
	{TestC, STACK_SIZE_TESTC, "TestC"}};
	*/

PUBLIC	char task_stack[STACK_SIZE_TOTAL];



PUBLIC	irq_handler	irq_table[NR_IRQ];

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {sys_printx,
						       sys_sendrec};

