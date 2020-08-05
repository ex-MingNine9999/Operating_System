#include <mem/palloc.h>
#include <bitmap.h>
#include <type.h>
#include <round.h>
#include <mem/mm.h>
#include <synch.h>
#include <device/console.h>
#include <mem/paging.h>
#include<mem/swap.h>

/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.  
   */
/* pool for memory */
struct memory_pool
{
	struct lock lock;                   
	struct bitmap *bitmap; 		// memory pool's first addr
	uint32_t *addr;             // memory pool's last addr next addr  
};
/* kernel heap page struct */
struct khpage{
	uint16_t page_type;
	uint16_t nalloc;			//the number of alloc
	uint32_t used_bit[4];
	struct khpage *next;
};

/* free list */
struct freelist{
	struct khpage *list;
	int nfree;
};

//static struct khpage *khpage_list;
//static struct freelist freelist;

static struct memory_pool mem_pool[2];
static uint32_t page_alloc_index[2];
// [0] = kernel pool, [1] = user pool


/* Initializes the page allocator. */

	void
init_palloc (void) 
{
	/* initialize */
	mem_pool[0].bitmap = create_bitmap((USER_POOL_START - KERNEL_ADDR) / PAGE_SIZE, (void *)KERNEL_ADDR, 8);
	set_bitmap(mem_pool[0].bitmap, 0, true);	// create kernel pool
	mem_pool[0].addr = (uint32_t *)USER_POOL_START;
	page_alloc_index[0] = 1;

	mem_pool[1].bitmap = create_bitmap((RKERNEL_HEAP_START - USER_POOL_START) / PAGE_SIZE, (void *)USER_POOL_START, 8);
	set_bitmap(mem_pool[1].bitmap, 0, true);	// create user pool
	mem_pool[1].addr = (uint32_t *)RKERNEL_HEAP_START;
	page_alloc_index[1] = 1;

}



/* Obtains and returns a group of PAGE_CNT contiguous free pages.
   */
/*
	uint32_t *
palloc_get_multiple_page (size_t page_cnt)
{
	void *pages = NULL;
	struct khpage *khpage = freelist.list;
	struct khpage *prepage = freelist.list;
	size_t page_idx;

	if (page_cnt == 0)
		return NULL;

	while(khpage != NULL){
		if(khpage->nalloc == page_cnt){
			page_idx = ((uint32_t)khpage - (uint32_t)khpage_list)/sizeof(struct khpage);
			pages = (void*)(VKERNEL_HEAP_START + page_idx * PAGE_SIZE);

			if(prepage == khpage){
				freelist.list = khpage->next;
				freelist.nfree--;
				break;
			}else{
				prepage->next = khpage->next;
				freelist.nfree--;
				break;
			}
				   	
		}
		prepage = khpage;
		khpage = khpage->next;
	}

	if(pages == NULL){
		pages = (void*)(VKERNEL_HEAP_START + page_alloc_index * PAGE_SIZE);
		page_alloc_index += page_cnt;
	}

	if (pages != NULL) 
	{
		memset (pages, 0, PAGE_SIZE * page_cnt);
	}

	return (uint32_t*)pages; 
}
*/

void* palloc_get_multiple_page(enum palloc_flags flags, size_t page_cnt)
{
	void *pages = NULL;
	size_t page_idx;
	
	if (page_cnt == 0) {
		return NULL;
	}

	page_idx = find_set_bitmap(mem_pool[flags].bitmap, page_alloc_index[flags], page_cnt, false);
	pages = (void *)(page_idx * PAGE_SIZE + (size_t)mem_pool[flags].bitmap);
	
	if ((uint32_t)pages + PAGE_SIZE * page_cnt >= (uint32_t)mem_pool[flags].addr) {
		return NULL;
	}

	if (page_alloc_index[flags] == page_idx) {
		page_alloc_index[flags] += page_cnt;
	}

	if (pages != NULL) {
		memset(pages, 0, PAGE_SIZE * page_cnt);
	}

	return pages;
}

/* Obtains a single free page and returns its address.
   */
/*
	uint32_t *
palloc_get_one_page (void) 
{
	return palloc_get_multiple_page (1);
}
*/

void *palloc_get_one_page(enum palloc_flags flags)
{
	return palloc_get_multiple_page(flags, 1);
}


/* Frees the PAGE_CNT pages starting at PAGES. */
/*
	void
palloc_free_multiple_page (void *pages, size_t page_cnt) 
{
	struct khpage *khpage = freelist.list;
	size_t page_idx = (((uint32_t)pages - VKERNEL_HEAP_START) / PAGE_SIZE);

	if (pages == NULL || page_cnt == 0)
		return;

	if(khpage == NULL){
		freelist.list = khpage_list + page_idx;
		freelist.list->nalloc = page_cnt;
		freelist.list->next = NULL;
	}
	else{

		while(khpage->next != NULL){
			khpage = khpage->next;
		}

		khpage->next = khpage_list + page_idx;
		khpage->next->nalloc = page_cnt;
		khpage->next->next = NULL;
	}

	freelist.nfree++;
}
*/
void palloc_free_multiple_page(void *pages, size_t page_cnt)
{
	size_t i;
	uint32_t page_idx;
	enum palloc_flags flags;

	if (pages == NULL || page_cnt == 0) {
		 return;
	}
	//pages = ((uint32_t)pages & ~PAGE_FLAG_PRESENT);

	flags = (uint32_t)pages < USER_POOL_START ? kernel_area : user_area;
	page_idx = ((uint32_t)pages - (uint32_t)mem_pool[flags].bitmap) / PAGE_SIZE;

	for (i = 0; i < page_cnt; i++) {
		set_bitmap(mem_pool[flags].bitmap, page_idx + i, false);
	}

	if (page_alloc_index[flags] > page_idx) {
		page_alloc_index[flags] = page_idx;
	}
}

/* Frees the page at PAGE. */
	void
palloc_free_one_page (void *page) 
{
	palloc_free_multiple_page (page, 1);
}


void palloc_pf_test(void)
{
	uint32_t *one_page1 = palloc_get_one_page(user_area);
	uint32_t *one_page2 = palloc_get_one_page(user_area);
	uint32_t *two_page1 = palloc_get_multiple_page(user_area,2);
	uint32_t *three_page;
	printk("one_page1 = %x\n", one_page1); 
	printk("one_page2 = %x\n", one_page2); 
	printk("two_page1 = %x\n", two_page1);

	printk("=----------------------------------=\n");
	palloc_free_one_page(one_page1);
	palloc_free_one_page(one_page2);
	palloc_free_multiple_page(two_page1,2);

	one_page1 = palloc_get_one_page(user_area);
	one_page2 = palloc_get_one_page(user_area);
	two_page1 = palloc_get_multiple_page(user_area,2);

	printk("one_page1 = %x\n", one_page1);
	printk("one_page2 = %x\n", one_page2);
	printk("two_page1 = %x\n", two_page1);

	printk("=----------------------------------=\n");
	palloc_free_multiple_page(one_page2, 3);
	one_page2 = palloc_get_one_page(user_area);
	three_page = palloc_get_multiple_page(user_area,3);

	printk("one_page1 = %x\n", one_page1);
	printk("one_page2 = %x\n", one_page2);
	printk("three_page = %x\n", three_page);

	palloc_free_one_page(one_page1);
	palloc_free_one_page(three_page);
	three_page = (uint32_t*)((uint32_t)three_page + 0x1000);
	palloc_free_one_page(three_page);
	three_page = (uint32_t*)((uint32_t)three_page + 0x1000);
	palloc_free_one_page(three_page);
	palloc_free_one_page(one_page2);
}
