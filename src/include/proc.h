
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#define MSG_CHAR		0

#define		PROC_MSG_MAX	256

typedef struct MSG{
	int type;
	int param1;
	int param2;
}MSG;
typedef struct s_proc_list
{
	struct MSG	data[PROC_MSG_MAX];	/* TTY 输入缓冲区 */
	struct MSG*	p_head;		/* 指向缓冲区中下一个空闲位置 */
	struct MSG*	p_tail;		/* 指向键盘任务应处理的键值 */
	int	count;				/* 缓冲区中已经填充了多少 */
}PROC_MSG_LIST;

struct stackframe {	/* proc_ptr points here				↑ Low			*/
	u32	gs;		/* ┓						│			*/
	u32	fs;		/* ┃						│			*/
	u32	es;		/* ┃						│			*/
	u32	ds;		/* ┃						│			*/
	u32	edi;		/* ┃						│			*/
	u32	esi;		/* ┣ pushed by save()				│			*/
	u32	ebp;		/* ┃						│			*/
	u32	kernel_esp;	/* <- 'popad' will ignore it			│			*/
	u32	ebx;		/* ┃						↑栈从高地址往低地址增长*/
	u32	edx;		/* ┃						│			*/
	u32	ecx;		/* ┃						│			*/
	u32	eax;		/* ┛						│			*/
	u32	retaddr;	/* return address for assembly code save()	│			*/
	u32	eip;		/*  ┓						│			*/
	u32	cs;		/*  ┃						│			*/
	u32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	u32	esp;		/*  ┃						│			*/
	u32	ss;		/*  ┛						┷High			*/
};


struct proc {
	struct stackframe regs;    /* process registers saved in stack frame */

	u16 ldt_sel;               /* gdt selector giving ldt base and limit */
	struct descriptor ldts[LDT_SIZE]; /* local descs for code and data */
	u32	pageDirBase;
    int ticks;                 /* remained ticks */
    int priority;

	u32 pid;                   /* process id passed in from MM */
	char name[16];		   /* name of the process */

	int  p_flags;              /**
				    * process flags.
				    * A proc is runnable iff p_flags==0
				    */

	MESSAGE * p_msg;
	int p_recvfrom;
	int p_sendto;

	int has_int_msg;           /**
				    * nonzero if an INTERRUPT occurred when
				    * the task is not ready to deal with it.
				    */

	struct proc * q_sending;   /**
				    * queue of procs sending messages to
				    * this proc
				    */
	struct proc * next_sending;/**
				    * next proc in the sending
				    * queue (q_sending)
				    */
	int p_parent;	//pid of parent
	int nr_tty;


	u32	filp[MAX_PROC_FILES];

	u32 vAddrHeapStart;

	PROC_MSG_LIST* pl;
};

struct task {
	task_f	initial_eip;
	int	stacksize;
	char	name[32];
};

//#define proc2pid(x) (x - proc_table)
int proc2pid(struct proc* p);

/* Number of tasks & procs */
#define NR_TASKS	8
#define NR_PROCS	32
#define NR_NATIVE_PROCS 		1
#define NR_ALL_PROCS (NR_TASKS + NR_PROCS)

#define FIRST_PROC	proc_table[0]
#define LAST_PROC	proc_table[NR_TASKS + NR_PROCS - 1]

#define PROCS_BASE		0x000000 	/*10M*/
#define PROC_IMAGE_SIZE_DEFAULT	0x100000		/* 1 M*/
#define PROC_ORIGIN_STACK			0x400		/*1 KB */

/* stacks of tasks */
#define STACK_SIZE_DEFAULT	0X4000
#define STACK_SIZE_TTY			STACK_SIZE_DEFAULT
#define STACK_SIZE_SYS			STACK_SIZE_DEFAULT
#define STACK_SIZE_HD				STACK_SIZE_DEFAULT
#define STACK_SIZE_FS				STACK_SIZE_DEFAULT
#define STACK_SIZE_MM			STACK_SIZE_DEFAULT
#define STACK_SIZE_GUI			STACK_SIZE_DEFAULT
#define STACK_SIZE_INIT			STACK_SIZE_DEFAULT
#define STACK_SIZE_MOUSE			STACK_SIZE_DEFAULT

//#define STACK_SIZE_TESTA		STACK_SIZE_DEFAULT
//#define STACK_SIZE_TESTB		STACK_SIZE_DEFAULT
//#define STACK_SIZE_TESTC		STACK_SIZE_DEFAULT

#define STACK_SIZE_TOTAL	(STACK_SIZE_TTY + \
				STACK_SIZE_SYS + \
				STACK_SIZE_HD + \
				STACK_SIZE_FS + \
				STACK_SIZE_MM + \
				STACK_SIZE_GUI + \
				STACK_SIZE_INIT + \
				STACK_SIZE_MOUSE)



