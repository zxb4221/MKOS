#include "../include/type.h"

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* 偁偒忣曬偺屄悢 */
	man->maxfrees = 0;		/* 忬嫷娤嶡梡丗frees偺嵟戝抣 */
	man->lostsize = 0;		/* 夝曻偵幐攕偟偨崌寁僒僀僘 */
	man->losts = 0;			/* 夝曻偵幐攕偟偨夞悢 */
	return;
}

unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* 廫暘側峀偝偺偁偒傪敪尒 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* free[i]偑側偔側偭偨偺偱慜傊偮傔傞 */
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* 峔憿懱偺戙擖 */
				}
			}
			return a;
		}
	}
	return 0; /* 偁偒偑側偄 */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 夝曻 */
{
	int i, j;
	/* 傑偲傔傗偡偝傪峫偊傞偲丄free[]偑addr弴偵暲傫偱偄傞傎偆偑偄偄 */
	/* 偩偐傜傑偢丄偳偙偵擖傟傞傋偒偐傪寛傔傞 */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 慜偑偁傞 */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 慜偺偁偒椞堟偵傑偲傔傜傟傞 */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 屻傠傕偁傞 */
				if (addr + size == man->free[i].addr) {
					/* 側傫偲屻傠偲傕傑偲傔傜傟傞 */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]偺嶍彍 */
					/* free[i]偑側偔側偭偨偺偱慜傊偮傔傞 */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* 峔憿懱偺戙擖 */
					}
				}
			}
			return 0; /* 惉岟廔椆 */
		}
	}
	/* 慜偲偼傑偲傔傜傟側偐偭偨 */
	if (i < man->frees) {
		/* 屻傠偑偁傞 */
		if (addr + size == man->free[i].addr) {
			/* 屻傠偲偼傑偲傔傜傟傞 */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 惉岟廔椆 */
		}
	}
	/* 慜偵傕屻傠偵傕傑偲傔傜傟側偄 */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]傛傝屻傠傪丄屻傠傊偢傜偟偰丄偡偒傑傪嶌傞 */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* 嵟戝抣傪峏怴 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 惉岟廔椆 */
	}
	/* 屻傠偵偢傜偣側偐偭偨 */
	man->losts++;
	man->lostsize += size;
	return -1; /* 幐攕廔椆 */
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}
