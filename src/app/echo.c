/*
 * echo.c
 *
 *  Created on: 2014年12月11日
 *      Author: zxb
 */
int printf(const char *fmt, ...);
int	printx(char* str);
void* malloc(unsigned int size);
int free(void* addr);
char szText[] = "global data!\n";
int main(int argc, char* argv[])
{
	int i;
	char* pBuffer;

	static char szStaticText[] = "static data!\n";
	printx("echo run!\n");
	printx(szText);
	printx(szStaticText);

	pBuffer = malloc(208*1024*1024);
	strcpy(pBuffer,"malloc data!\n");
	printx(pBuffer);
	free(pBuffer);


	pBuffer = malloc(1020*1024);
	for(i = 0; i < 3; i++)
		pBuffer[i] = 'Z';
	pBuffer[i] = '\n';
	pBuffer[i+1] = '\0';
	printx(pBuffer);
	free(pBuffer);

	int re = (int)(100*1.0/11);
	printf("3+4=%d\n",3+4);
	//printf("double:%d\n",(int)re);


	return 0;
}



