	AddrStart				equ 0x7c00		;扇区被加载到内存的绝对地址
	AddrOffsetStack			equ 0x7bff		;实模式栈顶，向低地址端生长
	AddrOfPartitionStart 	equ 446d		;主分区表相对于MBR头的偏移

	AddrOfMBRParam			equ 0x8000				;磁盘参数结构体地址，前2个字节是这个结构的大小，通过int 0x13调用，返回磁盘参数信息
	infomationFlags			equ AddrOfMBRParam+2h	;信息标志
	NumOfCylinders			equ AddrOfMBRParam+4h   ;磁道号
	NumOfHeads				equ AddrOfMBRParam+8h	;磁头号
	NumOfSectorsPerTrack	equ	AddrOfMBRParam+12d  ;每条磁道上的扇区数
	NumOfTotalSectors		equ AddrOfMBRParam+10h	;总扇区数
	BytesPerSector			equ AddrOfMBRParam+18h	;每个扇区的字节数


	AddrOfDiskBuffer		equ	0x8020					;磁盘地址包结构地址，用来读取磁盘扇区
	SizeOfPacket			equ AddrOfDiskBuffer		;包结构大小
	Reserved				equ AddrOfDiskBuffer+0x1	;保留
	NumOfBlocksToTransfer	equ AddrOfDiskBuffer+0x2	;读取的扇区数
	AddrOfDiskAddr			equ AddrOfDiskBuffer+0x4	;存放地址
	AddrOfDiskSec			equ AddrOfDiskBuffer+0x8	;8bytes，开始的绝对扇区号


	AddrOfExistSectors		equ 0x8000+0x30				;FAT32分区之前已经存在的扇区数

	AddrNextClusterRootDir	equ 0x8000+0x40



	AddrOfDBR				equ	0x8200					;DBR扇区读入内存的地址 40KB处
	SectorsPerCluster		equ AddrOfDBR+13d
	ReservedSectors			equ AddrOfDBR+14d
	FATCount				equ AddrOfDBR+10h
	SectorsPerFAT			equ AddrOfDBR+24h
	ClusterOfRootDir		equ AddrOfDBR+0x2c

	AddrOfRootDir			equ 0x8400	;fat32 根目录扇区地址

	AddrOfFAT				equ 0x8c00	;FAT扇区加载地址
	AddrOfOsloader			equ 0x9000	;OSLOADER.BIN 加载地址


	org AddrStart
	mov ax,cs
	mov ds,ax
	mov es,ax
	mov ss,ax

	mov sp,	AddrOffsetStack				;初始化栈ss:sp


	mov word [AddrOfMBRParam], 0x1A		;磁盘参数结构大小
	mov	ah,48h							;ah=48H 查询参数
	mov dl,80h							;dl=0x80 指定磁盘号
	mov si,AddrOfMBRParam				;DS:SI 指定参数缓存地址
	int 13h

	;枚举4个分区项，找到一个主分区
	mov cx,4
	mov bx,0x7c00+AddrOfPartitionStart
seekActivePartition:					;从主分区表中找到活动分区
	cmp byte [bx],80h
	je readFAT32_BR
	add bx,16
	loop seekActivePartition

	jmp $

readFAT32_BR:							;读取FAT32分区引导记录

	;将FAT32分区前已使用的扇区数复制到指定地址
	add bx,0x8							;根据分区表项，已使用的扇区数相对于分区表项头的偏移为0x8
	mov si,bx
	mov di,AddrOfExistSectors
	movsd								;将DS：SI处的内容移动到DS：DI处，移动4个字节


	mov ax,[AddrOfExistSectors]
	mov si,[AddrOfExistSectors+2]
	mov di,AddrOfDBR
	call ReadSector						;读 DBR 扇区到 AddrOfDBR,;ReadSector读指定的扇区，ax＝扇区号，si 为已存在的扇区数，ds:di=目的地址
	mov ax,[ClusterOfRootDir]
	mov word [AddrNextClusterRootDir],ax


	;读FAT32根目录区
	mov di,AddrOfRootDir
	call ReadCluster					;读指定的簇,ax=簇号，ds:di=目的地址

AnalyDir:								;分析FAT32根目录
	mov si,AddrOfRootDir				;DS:SI=32字节目录项
	mov ax,16
	mul byte [SectorsPerCluster]
	mov cx,ax
goNextIsOsloader:
	call IsOsloader						;判断是否是OSLOADER.BIN文件
	cmp ax,1
	je findOsloader
	add si,32
	loop goNextIsOsloader
	;TODO 继续读根目录FAT，找到OSLOADER.BIN
	mov ax,[AddrNextClusterRootDir]
	call ReadFat

	cmp ax,0
	je	LabelNoOsloader
	mov [AddrNextClusterRootDir],bx
	mov ax,bx
	mov di,AddrOfRootDir
	call ReadCluster

	jmp AnalyDir

LabelNoOsloader:
	hlt

findOsloader:
	;ds:(si+0x14,0x1a)为FAT号
	mov dx,AddrOfOsloader
	mov ax,[si+0x1a]
	mov di,AddrOfOsloader
	mov cx,ax
labelReadFat:
	call ReadCluster
	mov ax,cx
	call ReadFat
	cmp ax,1
	je labelReadOsloaderNext
	jmp AddrOfOsloader

labelReadOsloaderNext:
	push dx
	xor ax,ax
	mov al,[SectorsPerCluster]
	mul word [BytesPerSector]
	pop dx
	add dx,ax
	mov ax,bx
	mov cx,bx
	mov di,dx
	jmp labelReadFat


;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ReadFat:
	;读指定的FAT号数据ax=FAT号
	;返回 ax=1,bx=下一个簇号；ax=0,没有更多文件
	push dx
	mov bp,ax
	mov dx,0
	mov bx,128d
	div bx
	mov bp,dx
	cmp dx,0
	jne  labelSubNone
	sub ax,1
labelSubNone:

	mov bx,[AddrOfExistSectors]
	add bx,[ReservedSectors]
	add bx,ax
	mov ax,bx
	mov di,AddrOfFAT
	call ReadSector
	shl bp,2
	cmp word [AddrOfFAT+bp+2],0x0FFF
	je labelNoMoreFile
labelMoreFile:
	mov ax,1
	mov bx,[AddrOfFAT+bp]
	pop dx
	ret
labelNoMoreFile:
	xor ax,ax
	pop dx
	ret
;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ReadSector:
	;读指定的扇区，ax＝扇区号，ds:di=目的地址
	push dx
	mov byte [AddrOfDiskBuffer],0x10				;磁盘地址包结构大小
	mov byte [AddrOfDiskBuffer+1],0x0				;保留，为0
	mov word [AddrOfDiskBuffer+2],0x1				;读取的扇区数
	mov [AddrOfDiskBuffer+4],di										;
	mov [AddrOfDiskBuffer+6],ds
	mov [AddrOfDiskBuffer+8],ax


	mov ah,42h									;入口:AH＝42H
	mov dl,80h									;DL＝驱动器号（硬盘是80H）
	mov si,AddrOfDiskBuffer						;DS:SI＝磁盘地址包（即前面的数据结构的地址）
	int 13h
	pop dx
	ret
;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ReadCluster:
	;读指定的簇,ax=簇号，ds:di=目的地址
	push dx
	mov byte [AddrOfDiskBuffer],0x10
	mov bx,[AddrOfExistSectors]
	add bx,[ReservedSectors]
	add bx,[SectorsPerFAT]
	add bx,[SectorsPerFAT]


	sub ax,2
	mul byte [SectorsPerCluster]

	add ax,bx
	mov [AddrOfDiskSec],ax

	mov al,[SectorsPerCluster]
	mov byte [AddrOfDiskBuffer+2],al

	mov [AddrOfDiskAddr],di
	mov [AddrOfDiskAddr+2],ds

	mov si,AddrOfDiskBuffer


	mov ah,42h
	mov dl,80h
	int 13h

	pop dx
	ret
;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IsOsloader:
	;DS:SI=32字节目录项
	mov ax,[si+0xB]
	shr ax,5
	cmp ax,1
	je IsOsloaderNext
	ret
IsOsloaderNext:

	mov di,0

compareChar:
	mov bp,si
	add bp,di
	mov al,[bp]
	cmp al,[OSLOADER+di]
	je IsOsloaderNextOn
	xor ax,ax
	ret
IsOsloaderNextOn:
	inc di
	cmp di,11
	je IsOsloaderOK
	jmp compareChar
IsOsloaderOK:
	mov ax,1
	ret
;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


OSLOADER: db 'O','S','L','O','A','D','E','R','B','I','N'

times 	510-($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55					; 结束标志
