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
char szShellStack[STACK_SIZE_DEFAULT];
void Shell()
{
	MESSAGE msg;
	while (1)
	{
		printf("$:");

		send_recv(BOTH, TASK_TTY, &msg);
		if (msg.type == TTY_ENTER)
		{
			char* pFileName = (char*) msg.u.m2.m2p1;
			printf("%s\n", pFileName);
			if (strcmp(pFileName, "TESTFILE") == 0
					|| strcmp(pFileName, "TESTCALL") == 0
					|| strcmp(pFileName, "ECHO") == 0)
			{
				char szFileName[128] =
				{ 0 };

				memcpy(szFileName, pFileName, strlen(pFileName));
				reset_msg(&msg);
				msg.type = EXEC;
				msg.u.m2.m2p1 = (void*) szFileName;
				send_recv(BOTH, TASK_MM, &msg);
			}
		}
	}
}
