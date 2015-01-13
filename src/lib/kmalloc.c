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

#define PAGE_SIZE	4096
unsigned int get_free_page(unsigned int pageCount);
void free_page(unsigned int pAddr, unsigned int pageCount);

struct bucket_desc {	/* 16 bytes */
	void			*page;
	struct bucket_desc	*next;
	void			*freeptr;
	unsigned short		refcnt;
	unsigned short		bucket_size;
};

struct _bucket_dir {	/* 8 bytes */
	int			size;
	struct bucket_desc	*chain;
};

/*
 * The following is the where we store a pointer to the first bucket
 * descriptor for a given size.
 *
 * If it turns out that the Linux kernel allocates a lot of objects of a
 * specific size, then we may want to add that specific size to this list,
 * since that will allow the memory to be allocated more efficiently.
 * However, since an entire page must be dedicated to each specific size
 * on this list, some amount of temperance must be exercised here.
 *
 * Note that this list *must* be kept in order.
 */
struct _bucket_dir bucket_dir[] = {
	{ 16,	(struct bucket_desc *) 0},
	{ 32,	(struct bucket_desc *) 0},
	{ 64,	(struct bucket_desc *) 0},
	{ 128,	(struct bucket_desc *) 0},
	{ 256,	(struct bucket_desc *) 0},
	{ 512,	(struct bucket_desc *) 0},
	{ 1024,	(struct bucket_desc *) 0},
	{ 2048, (struct bucket_desc *) 0},
	{ 4096, (struct bucket_desc *) 0},
	{ 0,    (struct bucket_desc *) 0}};   /* End of list marker */

/*
 * This contains a linked list of free bucket descriptor blocks
 */
struct bucket_desc *free_bucket_desc = (struct bucket_desc *) 0;

/*
 * This routine initializes a bucket description page.
 */
void init_bucket_desc()
{
	struct bucket_desc *bdesc, *first;
	int	i;

	first = bdesc = (struct bucket_desc *) get_free_page(1);

	//printf("first:0x%x\n",first);

	if (!bdesc)
		panic("Out of memory in init_bucket_desc()");
	for (i = PAGE_SIZE/sizeof(struct bucket_desc); i > 1; i--) {
		bdesc->next = bdesc+1;
		bdesc++;
	}
	/*
	 * This is done last, to avoid race conditions in case
	 * get_free_page() sleeps and this routine gets called again....
	 */
	bdesc->next = free_bucket_desc;
	free_bucket_desc = first;
}

void *kmalloc(unsigned int len)
{
	struct _bucket_dir	*bdir;
	struct bucket_desc	*bdesc;
	void			*retval;

	/*
	 * First we search the bucket_dir to find the right bucket change
	 * for this request.
	 */
	for (bdir = bucket_dir; bdir->size; bdir++)
		if (bdir->size >= len)
			break;

	/*
	if (!bdir->size) {
		printf("malloc called with impossibly large argument (%d)\n",
			len);
		panic("malloc: bad arg");
	}
	*/
	if(!bdir->size){
		int pageCount = (len + 0xFFF) >> 12;
		return (void*)get_free_page(pageCount);
	}


	/*
	 * Now we search for a bucket descriptor which has free space
	 */
	//cli();	/* Avoid race conditions */
	disable_int();
	for (bdesc = bdir->chain; bdesc; bdesc = bdesc->next)
		if (bdesc->freeptr)
			break;
	/*
	 * If we didn't find a bucket with free space, then we'll
	 * allocate a new one.
	 */
	if (!bdesc) {
		char		*cp;
		int		i;

		if (!free_bucket_desc)
			init_bucket_desc();


		bdesc = free_bucket_desc;
		free_bucket_desc = bdesc->next;
		bdesc->refcnt = 0;
		bdesc->bucket_size = bdir->size;
		cp = (char*)(void *)get_free_page(1);
		bdesc->freeptr = cp;
		bdesc->page = cp;
		if (!cp)
			panic("Out of memory in kernel malloc()");
		/* Set up the chain of free objects */
		for (i=PAGE_SIZE/bdir->size; i > 1; i--) {
			*((char **) cp) = cp + bdir->size;
			cp += bdir->size;
		}
		*((char **) cp) = 0;
		bdesc->next = bdir->chain; /* OK, link it in! */
		bdir->chain = bdesc;
	}
	retval = (void *) bdesc->freeptr;
	bdesc->freeptr = *((void **) retval);
	bdesc->refcnt++;
	//sti();	/* OK, we're safe again */
	enable_int();
	return(retval);
}

/*
 * Here is the free routine.  If you know the size of the object that you
 * are freeing, then free_s() will use that information to speed up the
 * search for the bucket descriptor.
 *
 * We will #define a macro so that "free(x)" is becomes "free_s(x, 0)"
 */
void kfree_s(void *obj, int size)
{
	void		*page;
	struct _bucket_dir	*bdir;
	struct bucket_desc	*bdesc, *prev;
	bdesc = prev = 0;
	/* Calculate what page this object lives in */
	page = (void *)  ((unsigned int) obj & 0xfffff000);

	if(size > PAGE_SIZE){
		int pageCount = (size + 0xFFF) >> 12;
		free_page((unsigned int)page, pageCount);
		return;
	}

	/* Now search the buckets looking for that page */
	for (bdir = bucket_dir; bdir->size; bdir++) {
		prev = 0;
		/* If size is zero then this conditional is always false */
		if (bdir->size < size)
			continue;
		for (bdesc = bdir->chain; bdesc; bdesc = bdesc->next) {
			if (bdesc->page == page)
				goto found;
			prev = bdesc;
		}
	}
	panic("Bad address passed to kernel free_s()");
found:
	//cli(); /* To avoid race conditions */
	disable_int();
	*((void **)obj) = bdesc->freeptr;
	bdesc->freeptr = obj;
	bdesc->refcnt--;
	if (bdesc->refcnt == 0) {
		/*
		 * We need to make sure that prev is still accurate.  It
		 * may not be, if someone rudely interrupted us....
		 */
		if ((prev && (prev->next != bdesc)) ||
		    (!prev && (bdir->chain != bdesc)))
			for (prev = bdir->chain; prev; prev = prev->next)
				if (prev->next == bdesc)
					break;
		if (prev)
			prev->next = bdesc->next;
		else {
			if (bdir->chain != bdesc)
				panic("malloc bucket chains corrupted");
			bdir->chain = bdesc->next;
		}
		free_page((unsigned int) bdesc->page,1);
		bdesc->next = free_bucket_desc;
		free_bucket_desc = bdesc;

		//printf("F%x,%x\n",(unsigned int)&free_bucket_desc, (unsigned int)free_bucket_desc);
	}
	//sti();
	enable_int();
	return;
}
