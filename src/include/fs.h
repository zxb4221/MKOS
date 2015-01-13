#ifndef _FS_H

#define _FS_H

typedef struct FS_FILE{
	u32		uFileStartCluster;
	u32		uFileSize;
	u32		uCurPos;
	u32		uCount;
}FS_FILE;


#endif

