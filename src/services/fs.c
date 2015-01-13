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



PRIVATE void init_fs();
PRIVATE void do_open(MESSAGE* pMsg);
PRIVATE void do_read(MESSAGE* pMsg);
PRIVATE void read_file(MESSAGE* pMsg);
int ReadFileToMemory(int nFatNr, char* szBuffer);
int isFatEnd(int nFatNr, u32* pNextFat);
PRIVATE void do_close(MESSAGE* pMsg);

struct ROOT_DIR{
    char szFileName[8];
    char szExtendName[3];
    u8      attr;
    u8      reserved;
    u8      createTime;
    u16     fileCreateTime;
    u16     fileCreateDate;
    u16     fileAccessTime;
    u16     highCluster;
    u16     fileModifyTime;
    u16     fileModifyDate;
    u16     lowCluster;
    u32     fileSize;
};



PRIVATE FS_FILE fs_file_table[MAX_FS_FILES];

PRIVATE u32 offsetSectorCount;         		//分区开始处距硬盘开始的扇区数
PRIVATE u32  countOfPartition;            	//分区总扇区数
PRIVATE u16   bytesPerSector;               //每扇区字节数
PRIVATE u8     sectorsPerCluster;           //每簇的扇区数
PRIVATE u16   sectorsReserved;              //FAT1相对于分区起始的扇区数
PRIVATE u8      fatCount;                   //FAT个数
PRIVATE u32     sectorsOfFat;                    //每个FAT占用的扇区数
PRIVATE u32     nrClusterOfRootDir;         //根目录所在的第一个簇的簇号


//PRIVATE struct ROOT_DIR	cur_dir;

PUBLIC void task_fs()
{
	MESSAGE fs_msg;
	struct proc* pcaller;
	init_fs();

	while(1){
		send_recv(RECEIVE, ANY, &fs_msg);

		int msgtype = fs_msg.type;
		int src = fs_msg.source;
		pcaller = proc_table[src];

		switch (msgtype) {
		case OPEN:
			do_open(&fs_msg);
			break;
		case CLOSE:
			do_close(&fs_msg);
			break;
		case READ:
			do_read(&fs_msg);
			break;
		case WRITE:
			//fs_msg.CNT = do_rdwt();
			break;
		case UNLINK:
			//fs_msg.RETVAL = do_unlink();
			break;
		case RESUME_PROC:
			//src = fs_msg.PROC_NR;
			break;
		case FORK:
            fs_msg.RETVAL = 0;
			//fs_msg.RETVAL = fs_fork();
			break;
		case EXIT:
			//fs_msg.RETVAL = fs_exit();
			break;
		case LSEEK:
			//fs_msg.OFFSET = do_lseek();
			break;
		case STAT:
			//fs_msg.RETVAL = do_stat();
			break;
        case READ_FILE:
            read_file(&fs_msg);
            break;
		default:
			dump_msg("FS::unknown message:", &fs_msg);
			assert(0);
			break;
        }
        if (fs_msg.type != SUSPEND_PROC) {
                fs_msg.type = SYSCALL_RET;
                send_recv(SEND, src, &fs_msg);
            }
		}
}
PRIVATE void do_close(MESSAGE* pMsg)
{
	u32 flip = pMsg->u.m3.m3i1;
	if(flip >= MAX_FS_FILES)
		panic("fs do_close flip error,flip:0x%x\n", flip);
	if(!fs_file_table[flip].uCount)
		panic("fs do_close flip error,fs_file_table.uCount is already 0\n");

	fs_file_table[flip].uCount--;
}

PRIVATE void do_open(MESSAGE* pMsg)
{
	struct ROOT_DIR* pDir;
	char szBuffer[512];
	char* szFileName = pMsg->u.m2.m2p1;
	//char* szDesBuffer = pMsg->u.m2.m2p2;
	int i, j;
	int nCurFat;
	int nNextFat;
	MESSAGE msg;


	//read root dir
	reset_msg(&msg);
	msg.type = DEV_READ;
	msg.DEVICE = 0;
	msg.POSITION = (offsetSectorCount + sectorsReserved
			+ sectorsOfFat * fatCount) * SECTOR_SIZE;
	msg.CNT = SECTOR_SIZE;
	msg.PROC_NR = TASK_FS;
	msg.BUF = (void*) szBuffer;
	send_recv(BOTH, TASK_HD, &msg);

	pDir = (struct ROOT_DIR*) szBuffer;

	for (i = 0; i < 128; ++i)
	{
		for(j = 7; j >= 0; j--)
		{
			if(pDir->szFileName[j] == ' ')
				pDir->szFileName[j] = '\0';
			else
				break;
		}
		if(!strcmp_s(pDir->szFileName, szFileName,8))
		{
			break;
		}
		pDir++;
	}
	//printf("%s\n", pDir->szFileName);
	//TODO根目录遍历有问题
	//memcpy(&cur_dir, pDir, sizeof(struct ROOT_DIR));
	if (i < 128)
		pMsg->u.m3.m3i1 = pDir->fileSize;
	else
		printf("Not find file:%s\n",szFileName );

	for(i = 0; i < MAX_FS_FILES; ++i)
	{
		if(!fs_file_table[i].uCount)
		{
			fs_file_table[i].uCount++;
			fs_file_table[i].uCurPos = 0;
			fs_file_table[i].uFileSize = pDir->fileSize;
			fs_file_table[i].uFileStartCluster = (pDir->highCluster << 16) | (pDir->lowCluster);
			pMsg->u.m3.m3i2 = i;
			return;
		}
	}


	return;

}

PRIVATE void do_read(MESSAGE* pMsg)
{
	char* szDesBuffer = pMsg->u.m3.m3p1;
	char* szOrg = szDesBuffer;
	int size = pMsg->u.m3.m3i1;
	int offsetInFile = pMsg->u.m3.m3i2;
	u32 flip = pMsg->u.m3.m3i3;

	int nCountFatSkip = offsetInFile / SECTOR_SIZE;
	int nByteRest = offsetInFile % SECTOR_SIZE;

	u32 nCurFat = fs_file_table[flip].uFileStartCluster;
	int nNextFat;

	while ((nCountFatSkip > 0) && !isFatEnd(nCurFat, &nNextFat))
	{
		nCountFatSkip--;
		nCurFat = nNextFat;

	}

	do
	{
		size -= ReadFileToMemory(nCurFat, szDesBuffer);
		szDesBuffer += SECTOR_SIZE;
		if(isFatEnd(nCurFat, &nNextFat) || size <= 0)
			break;
		else
			nCurFat = nNextFat;
	}while (1);

	if (nByteRest)
		memcpy(szOrg, szOrg + nByteRest, pMsg->u.m3.m3i1);


}
PRIVATE void read_file(MESSAGE* pMsg)
{
    struct ROOT_DIR* pDir;
    char szBuffer[512];
    char* szFileName = pMsg->u.m2.m2p1;
    char* szDesBuffer = pMsg->u.m2.m2p2;
    int i,j;
    int nCurFat;
    int nNextFat;

    //printf(szFileName);
    //printf("\n");
    MESSAGE msg;
    /*
    //open hd
	reset_msg(&msg);
	msg.type = DEV_OPEN;
	send_recv(BOTH, TASK_HD, &msg);
	*/
    //read root dir
    reset_msg(&msg);
    msg.type = DEV_READ;
    msg.DEVICE = 0;
    msg.POSITION = (offsetSectorCount+sectorsReserved+sectorsOfFat*fatCount)*SECTOR_SIZE;
    msg.CNT = SECTOR_SIZE;
    msg.PROC_NR = TASK_FS;
    msg.BUF = (void*)szBuffer;
    send_recv(BOTH, TASK_HD, &msg);


    pDir = (struct ROOT_DIR*)szBuffer;
    for(i = 0; i < 128;++i)
    {
        for(j = 0; j < 8; ++j)
            {
                if(pDir->szFileName[j] != szFileName[j])
                    break;
            }
            if(j >= 8)
            {
                    //printf("Find it\n");
                    break;
                }
                pDir++;
    }

    nCurFat = pDir->lowCluster;
    while(!isFatEnd(nCurFat, &nNextFat))
    {
        ReadFileToMemory(nCurFat, szDesBuffer);
        nCurFat = nNextFat;
        szDesBuffer += SECTOR_SIZE;
    }

    /*
    reset_msg(&msg);
	msg.type = DEV_CLOSE;
	send_recv(BOTH, TASK_HD, &msg);
	*/



}
int ReadFileToMemory(int nFatNr, char* szBuffer)
{
     MESSAGE msg;
	reset_msg(&msg);
    msg.type = DEV_READ;
    msg.DEVICE = 0;
    msg.POSITION = (offsetSectorCount+sectorsReserved+sectorsOfFat*fatCount)*SECTOR_SIZE + (nFatNr - 2)*sectorsPerCluster*bytesPerSector;
    msg.CNT = SECTOR_SIZE;
    msg.PROC_NR = TASK_FS;
    msg.BUF = (void*)szBuffer;
    send_recv(BOTH, TASK_HD, &msg);

    return SECTOR_SIZE;
}
int isFatEnd(int nFatNr, u32* pNextFat)
{
    char szBuffer[SECTOR_SIZE] = {0};
    int FatSectorOffset;
    MESSAGE msg;
    u32 uFat = 0;
    FatSectorOffset = nFatNr/128;

	reset_msg(&msg);
    msg.type = DEV_READ;
    msg.DEVICE = 0;
    msg.POSITION = (offsetSectorCount+sectorsReserved+FatSectorOffset)*SECTOR_SIZE;
    msg.CNT = SECTOR_SIZE;
    msg.PROC_NR = TASK_FS;
    msg.BUF = (void*)szBuffer;
    send_recv(BOTH, TASK_HD, &msg);
    uFat = *((u32*)(szBuffer + (nFatNr%128)*4));
    if(uFat == 0x0FFFFFFF)
        return 1;
    else
        {
            *pNextFat = uFat;
            return 0;
        }
}

PRIVATE void init_fs()
{
    char szBuffer[512] = {0};
    u8* pActive = 0;
    int i;

    MESSAGE msg;
	reset_msg(&msg);
	msg.type = DEV_OPEN;
	send_recv(BOTH, TASK_HD, &msg);

	reset_msg(&msg);
    msg.type = DEV_READ;
    msg.DEVICE = 0;
    msg.POSITION = 0;
    msg.CNT = 512;
    msg.PROC_NR = TASK_FS;
    msg.BUF = (void*)szBuffer;
    send_recv(BOTH, TASK_HD, &msg);
    pActive = (u8*)(szBuffer+0x1BE);
    for(i = 0; i < 4; ++i)
    {
        if(*pActive == 0x80)
            break;

            pActive += 16;
    }
    if(i >= 4)
    {
        printf("Can‘t find active partition!\n");
        return;
    }

    offsetSectorCount = *((u32*)(pActive + 0x8));
    countOfPartition = *((u32*)(pActive + 0xC));

    reset_msg(&msg);
    msg.type = DEV_READ;
    msg.DEVICE = 0;
    msg.POSITION = 512*offsetSectorCount;
    msg.CNT = 512;
    msg.PROC_NR = TASK_FS;
    msg.BUF = (void*)szBuffer;
    send_recv(BOTH, TASK_HD, &msg);

    /*
	reset_msg(&msg);
	msg.type = DEV_CLOSE;
	send_recv(BOTH, TASK_HD, &msg);
	*/

   bytesPerSector = *((u16*)(szBuffer + 0xB));
   sectorsPerCluster = *((u8*)(szBuffer + 0xD));
   sectorsReserved = *((u16*)(szBuffer + 0xE));
   fatCount = *((u8*)(szBuffer + 0x10));
   sectorsOfFat = *((u32*)(szBuffer + 0x24));
   nrClusterOfRootDir =  *((u32*)(szBuffer + 0x2C));


    /*
    printf("offsetPartition:0X%x\n", offsetSectorCount);
    printf("countOfPartition:0X%x\n", countOfPartition);
    printf("bytesPerSector:0X%x\n", bytesPerSector);
    printf("sectorsPerCluster:0X%x\n", sectorsPerCluster);
    printf("sectorsReserved:0X%x\n", sectorsReserved);
    printf("fatCount:0X%x\n", fatCount);
    printf("sectorsOfFat:0X%x\n", sectorsOfFat);
    printf("nrClusterOfRootDir:0X%x\n", nrClusterOfRootDir);
    */

   memset(fs_file_table, 0, sizeof(FS_FILE)*MAX_FS_FILES);
}
