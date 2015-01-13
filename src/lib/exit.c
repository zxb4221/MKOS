/*
 * exit.c
 *
 *  Created on: 2014年12月11日
 *      Author: zxb
 */

#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/fs.h"
#include "../include/proc.h"
#include "../include/tty.h"
#include "../include/console.h"
#include "../include/global.h"

#include "../include/proto.h"


void* malloc(unsigned int size)
{
	MESSAGE msg;
	memset(&msg, 0, sizeof(MESSAGE));
	msg.type = MALLOC;
	msg.u.m3.m3i1 = size;
	send_recv(BOTH, TASK_MM, &msg);
	return msg.u.m3.m3p1;
}

int free(void* addr)
{
	MESSAGE msg;
	memset(&msg, 0, sizeof(MESSAGE));
	msg.type = FREE;
	msg.u.m3.m3p1 = addr;
	send_recv(BOTH, TASK_MM, &msg);
	return msg.u.m3.m3i1;
}
void exit(int status)
{
	MESSAGE msg;
	memset(&msg, 0, sizeof(MESSAGE));
	msg.type = EXIT;
	msg.u.m1.m1i1 = status;
	send_recv(BOTH, TASK_MM, &msg);
	//assert(msg.type == SYSCALL_RET);
}



