BaseOfLoaderPhyAddr	equ 0x9000

BaseOfKernelFile	equ	 01000h	; KERNEL.BIN 被加载到的位置 ----  段地址
OffsetOfKernelFile	equ	     0h	; KERNEL.BIN 被加载到的位置 ---- 偏移地址

BaseOfKernelFilePhyAddr	equ	BaseOfKernelFile * 10h
KernelEntryPointPhyAddr	equ	0xA00000

PageDirBase		equ	200000h	; 页目录开始地址:			2M
PageTblBase		equ	201000h	; 页表开始地址:			2M + 4K

;----------------------------------------------------------------------------
; 分页机制使用的常量说明
;----------------------------------------------------------------------------
PG_P		EQU	1	; 页存在属性位
PG_RWR		EQU	0	; R/W 属性位值, 读/执行
PG_RWW		EQU	2	; R/W 属性位值, 读/写/执行
PG_USS		EQU	0	; U/S 属性位值, 系统级
PG_USU		EQU	4	; U/S 属性位值, 用户级
;----------------------------------------------------------------------------

DA_32		EQU	4000h	; 32 位段
DA_LIMIT_4K	EQU	0x8000	; 段界限粒度为 4K 字节

DA_DPL0		EQU	  00h	; DPL = 0
DA_DPL1		EQU	  20h	; DPL = 1
DA_DPL2		EQU	  40h	; DPL = 2
DA_DPL3		EQU	  60h	; DPL = 3

;存储段描述符类型
DA_DR	equ		90h
DA_DRW	equ		92h
DA_DRWA	equ		93h
DA_C	equ		98h
DA_CR	equ		9Ah
DA_CC0	equ		9Ch
DA_CC0R	equ		9Eh

AddrOfExistSectors		equ 0x8000+0x30
AddrVideoBuffer			equ 0x8000+0x34


AddrOfDBR				equ	0x8200					;DBR扇区读入内存的地址 40KB处
SectorsPerCluster		equ AddrOfDBR+13d
ReservedSectors			equ AddrOfDBR+14d
FATCount				equ AddrOfDBR+10h
SectorsPerFAT			equ AddrOfDBR+24h
ClusterOfRootDir		equ AddrOfDBR+0x2c

AddrOfMBRParam			equ 0x8000	;磁盘参数地址
NumOfCylinders			equ AddrOfMBRParam+4h
NumOfHeads				equ AddrOfMBRParam+8h
NumOfSectorsPerTrack	equ	AddrOfMBRParam+12d
NumOfTotalSectors		equ AddrOfMBRParam+10h
BytesPerSector			equ AddrOfMBRParam+18h

AddrOfRootDir			equ 0x9800	;fat32 根目录扇区地址
AddrOfFAT				equ 0x8c00	;FAT扇区加载地址

AddrNextClusterRootDir	equ 0x8000+0x40

AddrKernelBase			equ	0x1000
AddrKernelOffset		equ 0

;	;Base,Limit,Attr
;	Base:	dd
; 	Limit:	dd(low 20 bits available)
;	Attr:	dw(lower 4 bits of higher byte are always 0)

%macro Descriptor 3
	dw	%2 & 0FFFFh							; 段界限1
	dw	%1 & 0FFFFh							; 段基址1
	db	(%1 >> 16) & 0FFh					; 段基址2
	dw	((%2 >> 8) & 0F00h) | (%3 & 0F0FFh)	; 属性1 + 段界限2 + 属性2
	db	(%1 >> 24) & 0FFh					; 段基址3
%endmacro ; 共 8 字节

	org BaseOfLoaderPhyAddr
	jmp LABEL_START

[SECTION .gdt]
LABEL_GDT:				Descriptor 		0,			  0,		0
LABEL_DESC_CODE32:		Descriptor		0,		0fffffh, 		DA_CR  | DA_32 | DA_LIMIT_4K
LABEL_DESC_FLAT_RW:		Descriptor      0,      0fffffh, 		DA_DRW | DA_32 | DA_LIMIT_4K			; 0 ~ 4G
LABEL_DESC_VIDEO:		Descriptor	0B8000h,	0FFFFh,			DA_DRW | DA_DPL3						; 显存首地址
LABEL_DESC_VRAM:		Descriptor	0A0000h,	0FFFFFh,		DA_DRW | DA_32 | DA_LIMIT_4K						; 显存首地址
GdtLen		equ	$-LABEL_GDT
GdtPtr:		dw	GdtLen - 1
			dd	LABEL_GDT		; 基地址

SelectorCode32	equ	LABEL_DESC_CODE32 - LABEL_GDT
SelectorFlatRW	equ	LABEL_DESC_FLAT_RW	- LABEL_GDT
SelectorVideo	equ LABEL_DESC_VIDEO - LABEL_GDT

[SECTION .s16]
[BITS	16]
LABEL_START:
	;清屏
	mov ax,0600h
	mov	bx,0700h
	mov cx,0
	mov dx,0184fh
	int 10h

	mov cx,0x115
	mov di,AddrVideoBuffer
	mov ax,0x4F01
	int 0x10


	mov bx,0x4115 ;800*600 8:8:8
	mov ax,0x4f02
	int 0x10


	;显示Loading
	push 0h
	call DisplayString
	add sp,2

	call GetMemSize




	;读KERNELEX.ELF
	call ReadKernel

	;进入32位保护模式
	call EnterPM

	;不会执行到这里了！
	jmp AddrKernelBase:AddrKernelOffset
	hlt


EnterPM:

	; 初始化 32 位代码段描述符
	;xor	eax, eax
	;mov	ax, cs
	;shl	eax, 4
	;add	eax, LABEL_SEG_CODE32
	;mov	word [LABEL_DESC_CODE32 + 2], ax
	;shr	eax, 16
	;mov	byte [LABEL_DESC_CODE32 + 4], al
	;mov	byte [LABEL_DESC_CODE32 + 7], ah

	; 为加载 GDTR 作准备
	;xor	eax, eax
	;mov	ax, ds
	;shl	eax, 4
	;add	eax, LABEL_GDT		; eax <- gdt 基地址
	;mov	dword [GdtPtr + 2], eax	; [GdtPtr + 2] <- gdt 基地址

	; 加载 GDTR
	lgdt	[GdtPtr]

	; 关中断
	cli

	; 打开地址线A20
	in	al, 92h
	or	al, 00000010b
	out	92h, al

	;准备切换到保护模式
	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax

	; 真正进入保护模式
	jmp	dword SelectorCode32:LABEL_SEG_CODE32
	ret

GetMemSize:
	; 得到内存数
	mov	ebx, 0			; ebx = 后续值, 开始时需为 0
	mov	di, _MemChkBuf	; es:di 指向一个地址范围描述符结构（Address Range Descriptor Structure）
.MemChkLoop:
	mov	eax, 0E820h		; eax = 0000E820h
	mov	ecx, 20			; ecx = 地址范围描述符结构的大小
	mov	edx, 0534D4150h	; edx = 'SMAP'
	int	15h				; int 15h
	jc	.MemChkFail
	add	di, 20
	inc	dword [_dwMCRNumber]	; dwMCRNumber = ARDS 的个数
	cmp	ebx, 0
	jne	.MemChkLoop
	jmp	.MemChkOK
.MemChkFail:
	mov	dword [_dwMCRNumber], 0
.MemChkOK:
	ret


ReadKernel:
	;读根目录
	push 0h
	push word [ClusterOfRootDir]
	push ds
	push AddrOfRootDir
	call ReadCluster
	add sp,8

	mov ax,[ClusterOfRootDir]
	mov word [AddrNextClusterRootDir],ax

AnalyDir:
	mov si,AddrOfRootDir					;DS:SI=32字节目录项
	mov ax,16
	mul byte [SectorsPerCluster]
	mov cx,ax
goNextIsOsloader:
	push ds
	push si
	call IsKernel
	add sp,4

	cmp ax,1
	je yesok
	add si,32
	loop goNextIsOsloader
	;TODO 继续读根目录FAT，找到OSLOADER.BIN
	mov ax,[AddrNextClusterRootDir]

	push 0
	push ax
	push ds
	push AddrOfFAT

	call ReadFat
	add sp,8

	cmp ax,0
	je	LabelNoOsloader
	mov [AddrNextClusterRootDir],bx

	push 0
	push bx
	push ds
	push AddrOfRootDir
	call ReadCluster
	add sp,8

	jmp AnalyDir

LabelNoOsloader:
	push 1h
	call DisplayString
	add sp,2
	xor ax,ax
	ret


yesok:
	mov di,AddrKernelOffset
	mov ax,[si+0x1a]
	mov cx,ax
	push 0
	push ax
	push AddrKernelBase
	push AddrKernelOffset
labelReadFat:
	call ReadCluster
	add sp,8
	push 0
	push cx
	push ds
	push AddrOfFAT
	call ReadFat
	add sp,8
	cmp ax,1
	je labelReadOsloaderNext

	push 2h
	call DisplayString
	add sp,2

	mov ax,1
	ret

	jmp AddrKernelBase:AddrKernelOffset

	hlt


labelReadOsloaderNext:
	xor ax,ax
	mov al,[SectorsPerCluster]
	mul word [BytesPerSector]

	add di,ax
	mov cx,bx
	push 0
	push bx
	push AddrKernelBase
	push di

	jmp labelReadFat





	xor eax,eax
	mov ax,cs
	shl	eax,4
	add eax,LABEL_SEG_CODE32
	mov word [LABEL_DESC_CODE32+2],ax
	shr	eax,16
	mov byte [LABEL_DESC_CODE32+4],al
	mov byte [LABEL_DESC_CODE32+7],ah

	xor eax,eax
	mov	ax,ds
	shl	eax,4
	add	eax, LABEL_GDT
	mov	dword [GdtPtr+2],eax

	lgdt [GdtPtr]

	cli
	in al,92h
	or al,00000010b
	out 92h,al

	mov	eax,cr0
	or eax,1
	mov cr0,eax

	jmp dword SelectorCode32:0



KernelFileName		db	"KERNELEXELF"

;/////////////////////////////////////////////////
;分析是否是Kernel.elf文件,ss:sp+2=偏移地址，ss:sp+4=段地址
;段：偏移＝32字节目录项,ax=0不是，ax=1是
IsKernel:
	push bp
	mov bp,sp
	add bp,2

	push bx
	push es
	push si
	push di

	mov es,[ss:bp+4]
	mov bx,[ss:bp+2]

	mov ax,[es:bx+0xB]
	shr ax,5
	cmp ax,1
	je IsKernelNext
	mov ax,0
	jmp labelIsKernelRet
IsKernelNext:
	mov di,0
la:
	mov bp,si
	add bp,di
	mov al,[bp]
	cmp al,[KernelFileName+di]
	je IsKernelNextOn
	mov ax,0
	jmp labelIsKernelRet
IsKernelNextOn:
	inc di
	cmp di,11
	je IsKernelNextOnOn
	jmp la
IsKernelNextOnOn:
	mov ax,1
	jmp labelIsKernelRet

labelIsKernelRet:
	pop di
	pop si
	pop es
	pop bx
	pop bp
	ret
;/////////////////////////////////////////////////



MessageLength		equ	10
MessageLoad			db	"Loading   "
MessageNoKernel		db	"No Kernel!"
MessageKernelLoaded	db	"Kernelload"
;/////////////////////////////////////////////////
;函数显示字符串,ss:sp+2＝字符串索引
;/////////////////////////////////////////////////
DisplayString:
	mov bp,sp
	mov bx,[ss:bp+2]

	xor dx,dx
	mov	ax,	MessageLength
	mul bx
	add ax,MessageLoad
	mov bp,ax

	mov ax,ds
	mov es,ax
	mov cx,MessageLength
	mov ax,01301h
	mov bx,0007h
	mov dl,0
	mov dh,3
	int 10h
	ret
;/////////////////////////////////////////////////



;////////////////////////////////////////////////
;读扇区,ss:sp+2=偏移地址，ss:sp+4=段地址，ss:sp+6＝扇区号低16位，ss:sp+8＝扇区号高16位
ReadSector:
	push bp
	mov bp,sp
	add bp,2

	push bx
	push ax
	push dx
	push si




	sub sp,16d
	mov bx,sp

	mov byte [ss:bx],16d
	mov byte [ss:bx+1],0d
	mov word [ss:bx+2],1d

	mov ax,[ss:bp+2]
	mov [ss:bx+4],ax
	mov ax,[ss:bp+4]
	mov [ss:bx+6],ax

	mov ax,[ss:bp+6]
	mov [ss:bx+8],ax
	mov ax,[ss:bp+8]
	mov [ss:bx+10],ax;
	mov dword [ss:bx+12],0d

	mov si,bx

	mov ah,42h				;入口:AH＝42H
	mov dl,80h				;DL＝驱动器号（硬盘是80H）
	int 13h					;DS:SI＝磁盘地址包（即前面的数据结构的地址）

	add sp,16d

	pop si
	pop dx
	pop ax
	pop bx
	pop bp
	ret
;/////////////////////////////////////////////////////

;/////////////////////////////////////////////////////
;读簇,ss:sp+2=偏移地址，ss:sp+4=段地址，ss:sp+6＝簇号低16位，ss:sp+8＝簇号高16位
ReadCluster:
	push bp
	mov bp,sp
	add bp,2


	push ax
	push bx
	push dx
	push si




	sub sp,16d
	mov bx,sp
	mov byte [ss:bx],16d
	mov byte [ss:bx+1],0d

	mov al,[SectorsPerCluster]
	xor ah,ah
	mov [ss:bx+2],ax

	mov ax,[ss:bp+2]
	mov [ss:bx+4],ax
	mov ax,[ss:bp+4]
	mov [ss:bx+6],ax


	mov dx,[AddrOfExistSectors]
	add dx,[ReservedSectors]
	add dx,[SectorsPerFAT]
	add dx,[SectorsPerFAT]

	mov ax,[ss:bp+6]
	sub ax,2
	mul byte [SectorsPerCluster]

	add ax,dx
	mov [ss:bx+8],ax
	mov word [ss:bx+10],0d;
	mov dword [ss:bx+12],0d

	mov si,bx

	mov ah,42h
	mov dl,80h
	int 13h
	add sp,16d

	pop si
	pop dx
	pop bx
	pop ax
	pop bp
	ret
;/////////////////////////////////////////////////////


;/////////////////////////////////////////////////////
;读FAT,ss:sp+2=偏移地址，ss:sp+4=段地址，ss:sp+6＝FAT号低16位，ss:sp+8＝FAT号高16位
;返回 ax=1,bx=下一个簇号；ax=0,没有更多文件
ReadFat:
	push bp
	mov bp,sp
	add bp,2

	push es
	push si
	push di



	mov ax,[ss:bp+6]
	mov dx,0
	mov bx,128d
	div bx
	cmp dx,0
	jne  labelSubNone
	sub ax,1
labelSubNone:

	mov bx,[AddrOfExistSectors]
	add bx,[ReservedSectors]
	add bx,ax

	push 0h
	push bx
	mov ax,[ss:bp+4]
	push ax
	mov ax,[ss:bp+2]
	push ax
	call ReadSector
	add sp,8

	mov di,dx
	shl di,2
	mov es,[ss:bp+4]
	mov si,[ss:bp+2]
	add si,di
	cmp word [es:si+2],0x0FFF
	;TODO
	je labelNoMoreFile
labelMoreFile:
	mov ax,1
	mov bx,[es:si]
	jmp labeFatRet
labelNoMoreFile:
	mov ax,0
	jmp labeFatRet
labeFatRet:
	pop di
	pop si
	pop es
	pop bp
	ret
;/////////////////////////////////////////////////////

[SECTION .s32]; 32 位代码段. 由实模式跳入.
[BITS	32]
LABEL_SEG_CODE32:
	mov	ax,SelectorVideo
	mov gs,ax

	mov edi,(80*4+0)*2
	mov ah,0Ch
	mov al,'P'
	mov [gs:edi],ax

	mov ax,SelectorFlatRW
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov ss,ax
	mov esp,TopOfStack

	push szMemChkTitle
	call DispStr
	add	 esp,4

	call DispMemInfo

	;call SetupPaging

	call InitKernel
	;call CopyKernel

	jmp	SelectorCode32:(KernelEntryPointPhyAddr)	; 正式进入内核 *

	hlt
	jmp $


CopyKernel:
	push 0x100000						;size=1M
	mov eax,BaseOfKernelFilePhyAddr		;src
	push eax
	mov eax,KernelEntryPointPhyAddr		;des
	push eax
	call MemCpy
	add esp,12
	ret


; InitKernel ---------------------------------------------------------------------------------
; 将 KERNEL.BIN 的内容经过整理对齐后放到新的位置
; --------------------------------------------------------------------------------------------
InitKernel:	; 遍历每一个 Program Header，根据 Program Header 中的信息来确定把什么放进内存，放到什么位置，以及放多少。
	xor	esi, esi
	mov	cx, word [BaseOfKernelFilePhyAddr + 2Ch]; ┓ ecx <- pELFHdr->e_phnum
	movzx	ecx, cx					; ┛
	mov	esi, [BaseOfKernelFilePhyAddr + 1Ch]	; esi <- pELFHdr->e_phoff
	add	esi, BaseOfKernelFilePhyAddr		; esi <- OffsetOfKernel + pELFHdr->e_phoff
.Begin:
	mov	eax, [esi + 0]
	cmp	eax, 0				; PT_NULL
	jz	.NoAction
	push	dword [esi + 010h]		; size	┓
	mov	eax, [esi + 04h]		;	┃
	add	eax, BaseOfKernelFilePhyAddr	;	┣ ::memcpy(	(void*)(pPHdr->p_vaddr),
	push	eax				; src	┃		uchCode + pPHdr->p_offset,
	push	dword [esi + 08h]		; dst	┃		pPHdr->p_filesz;
	call	MemCpy				;	┃
	add	esp, 12				;	┛
.NoAction:
	add	esi, 020h			; esi += pELFHdr->e_phentsize
	dec	ecx
	jnz	.Begin

	ret
; InitKernel ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


; 启动分页机制 --------------------------------------------------------------
SetupPaging:
	; 根据内存大小计算应初始化多少PDE以及多少页表
	xor	edx, edx
	mov	eax, [dwMemSize]
	mov	ebx, 400000h	; 400000h = 4M = 4096 * 1024, 一个页表对应的内存大小
	div	ebx
	mov	ecx, eax	; 此时 ecx 为页表的个数，也即 PDE 应该的个数
	test	edx, edx
	jz	.no_remainder
	inc	ecx		; 如果余数不为 0 就需增加一个页表
.no_remainder:
	push	ecx		; 暂存页表个数

	; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.

	; 首先初始化页目录
	mov	ax, SelectorFlatRW
	mov	es, ax
	mov	edi, PageDirBase	; 此段首地址为 PageDirBase
	xor	eax, eax
	mov	eax, PageTblBase | PG_P  | PG_USU | PG_RWW
.1:
	stosd
	add	eax, 4096		; 为了简化, 所有页表在内存中是连续的.
	loop	.1

	; 再初始化所有页表
	pop	eax			; 页表个数
	mov	ebx, 1024		; 每个页表 1024 个 PTE
	mul	ebx
	mov	ecx, eax		; PTE个数 = 页表个数 * 1024
	mov	edi, PageTblBase	; 此段首地址为 PageTblBase
	xor	eax, eax
	mov	eax, PG_P  | PG_USU | PG_RWW
.2:
	stosd
	add	eax, 4096		; 每一页指向 4K 的空间
	loop	.2

	mov	eax, PageDirBase
	mov	cr3, eax
	mov	eax, cr0
	or	eax, 80000000h
	mov	cr0, eax
	jmp	short .3
.3:
	nop

	ret
; 分页机制启动完毕 ----------------------------------------------------------


; ------------------------------------------------------------------------
; 内存拷贝，仿 memcpy
; ------------------------------------------------------------------------
; void* MemCpy(void* es:pDest, void* ds:pSrc, int iSize);
; ------------------------------------------------------------------------
MemCpy:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	esi, [ebp + 12]	; Source
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; 判断计数器
	jz	.2		; 计数器为零时跳出

	mov	al, [ds:esi]		; ┓
	inc	esi			; ┃
					; ┣ 逐字节移动
	mov	byte [es:edi], al	; ┃
	inc	edi			; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:
	mov	eax, [ebp + 8]	; 返回值

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回
; MemCpy 结束-------------------------------------------------------------


; ------------------------------------------------------------------------
; 显示 AL 中的数字
; ------------------------------------------------------------------------
DispAL:
	push	ecx
	push	edx
	push	edi

	mov	edi, [dwDispPos]

	mov	ah, 0Fh			; 0000b: 黑底    1111b: 白字
	mov	dl, al
	shr	al, 4
	mov	ecx, 2
.begin:
	and	al, 01111b
	cmp	al, 9
	ja	.1
	add	al, '0'
	jmp	.2
.1:
	sub	al, 0Ah
	add	al, 'A'
.2:
	mov	[gs:edi], ax
	add	edi, 2

	mov	al, dl
	loop	.begin
	;add	edi, 2

	mov	[dwDispPos], edi

	pop	edi
	pop	edx
	pop	ecx

	ret
; DispAL 结束-------------------------------------------------------------



; ------------------------------------------------------------------------
; 显示一个整形数
; ------------------------------------------------------------------------
DispInt:
	mov	eax, [esp + 4]
	shr	eax, 24
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 16
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 8
	call	DispAL

	mov	eax, [esp + 4]
	call	DispAL

	mov	ah, 07h			; 0000b: 黑底    0111b: 灰字
	mov	al, 'h'
	push	edi
	mov	edi, [dwDispPos]
	mov	[gs:edi], ax
	add	edi, 4
	mov	[dwDispPos], edi
	pop	edi

	ret
; DispInt 结束------------------------------------------------------------

; ------------------------------------------------------------------------
; 显示一个字符串
; ------------------------------------------------------------------------
DispStr:
	push	ebp
	mov	ebp, esp
	push	ebx
	push	esi
	push	edi

	mov	esi, [ebp + 8]	; pszInfo
	mov	edi, [dwDispPos]
	mov	ah, 0Fh
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[dwDispPos], edi

	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
; DispStr 结束------------------------------------------------------------

; ------------------------------------------------------------------------
; 换行
; ------------------------------------------------------------------------
DispReturn:
	push	szReturn
	call	DispStr			;printf("\n");
	add	esp, 4

	ret
; DispReturn 结束---------------------------------------------------------


; 显示内存信息 --------------------------------------------------------------
DispMemInfo:
	push	esi
	push	edi
	push	ecx

	mov	esi, MemChkBuf
	mov	ecx, [dwMCRNumber]	;for(int i=0;i<[MCRNumber];i++) // 每次得到一个ARDS(Address Range Descriptor Structure)结构
.loop:					;{
	mov	edx, 5			;	for(int j=0;j<5;j++)	// 每次得到一个ARDS中的成员，共5个成员
	mov	edi, ARDStruct		;	{			// 依次显示：BaseAddrLow，BaseAddrHigh，LengthLow，LengthHigh，Type
.1:					;
	push	dword [esi]		;
	call	DispInt			;		DispInt(MemChkBuf[j*4]); // 显示一个成员
	pop	eax			;
	stosd				;		ARDStruct[j*4] = MemChkBuf[j*4];
	add	esi, 4			;
	dec	edx			;
	cmp	edx, 0			;
	jnz	.1			;	}
	call	DispReturn		;	printf("\n");
	cmp	dword [dwType], 1	;	if(Type == AddressRangeMemory) // AddressRangeMemory : 1, AddressRangeReserved : 2
	jne	.2			;	{
	mov	eax, [dwBaseAddrLow]	;
	add	eax, [dwLengthLow]	;
	cmp	eax, [dwMemSize]	;		if(BaseAddrLow + LengthLow > MemSize)
	jb	.2			;
	mov	[dwMemSize], eax	;			MemSize = BaseAddrLow + LengthLow;
.2:					;	}
	loop	.loop			;}
					;
	call	DispReturn		;printf("\n");
	push	szRAMSize		;
	call	DispStr			;printf("RAM size:");
	add	esp, 4			;
					;
	push	dword [dwMemSize]	;
	call	DispInt			;DispInt(MemSize);
	add	esp, 4			;

	pop	ecx
	pop	edi
	pop	esi
	ret
; ---------------------------------------------------------------------------



SegCode32Len	equ 	$-LABEL_SEG_CODE32

[SECTION .data]
ALIGN	32
LABEL_DATA:
_szMemChkTitle:				db		"BaseAddrL BaseAddrH LengthLow	LengthHigh Type",0Ah,0
_szRAMSize:					db		"RAM size:",0
_szReturn:					db		0Ah,0

_dwMCRNumber:				dd		0
_dwDispPos:					dd		(80*6+0)*2
_dwMemSize:					dd		0
_ARDStruct:
	_dwBaseAddrLow:			dd		0
	_dwBaseAddrHigh:		dd		0
	_dwLengthLow:			dd		0
	_dwLengthHigh:			dd		0
	_dwType:				dd		0
_MemChkBuf:	times	256		db		0


;; 保护模式下使用这些符号

szMemChkTitle	equ	_szMemChkTitle
szRAMSize		equ	_szRAMSize
szReturn		equ	_szReturn
dwDispPos		equ	_dwDispPos
dwMemSize		equ	_dwMemSize
dwMCRNumber		equ	_dwMCRNumber
ARDStruct		equ	_ARDStruct
dwBaseAddrLow	equ	_dwBaseAddrLow
dwBaseAddrHigh	equ	_dwBaseAddrHigh
dwLengthLow		equ	_dwLengthLow
dwLengthHigh	equ	_dwLengthHigh
dwType			equ	_dwType
MemChkBuf		equ	_MemChkBuf

; 堆栈就在数据段的末尾
StackSpace:	times	1000h	db	0
TopOfStack	equ	 $	; 栈顶

