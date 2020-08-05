#include <device/io.h>
#include <mem/mm.h>
#include <mem/paging.h>
#include <device/console.h>
#include <proc/proc.h>
#include <interrupt.h>
#include <mem/palloc.h>
#include <ssulib.h>
#include<device/ide.h>

uint32_t *PID0_PAGE_DIR;

intr_handler_func pf_handler;

//모든 함수는 수정이 가능가능

uint32_t scale_up(uint32_t base, uint32_t size)
{
	uint32_t mask = ~(size-1);
	if((base & mask) != 0)
		base = (base & mask) + size;
	return base;
}
//해당 코드를 사용하지 않고 구현해도 무관함
void pagememcpy(void* from, void* to, uint32_t len)
{
	uint32_t *p1 = (uint32_t*)from;
	uint32_t *p2 = (uint32_t*)to;
	int i, e;

	e = len / sizeof(uint32_t);
	for(i = 0; i < e; i++) {
		if ((uint32_t)(p2 + i) > RKERNEL_HEAP_START) {
			return;
		}
		p2[i] = p1[i];
	}

	e = len % sizeof(uint32_t);
	if( e != 0)
	{
		uint8_t *p3 = (uint8_t*)(p1 + i);
		uint8_t *p4 = (uint8_t*)(p2 + i);
		for(i = 0; i < e; i++) {
			if ((uint32_t)(p4 + i) > RKERNEL_HEAP_START) {
				return;
			}
			p4[i] = p3[i];
		}
	}
}

uint32_t scale_down(uint32_t base, uint32_t size)
{
	uint32_t mask = ~(size-1);
	if((base & mask) != 0)
		base = (base & mask) - size;
	return base;
}

void init_paging()
{
	uint32_t *page_dir = palloc_get_one_page(kernel_area);
	uint32_t *page_tbl = palloc_get_one_page(kernel_area);
	//	page_dir = VH_TO_RH(page_dir);
	//	page_tbl = VH_TO_RH(page_tbl);
	PID0_PAGE_DIR = page_dir;

	int NUM_PT, NUM_PE;
	uint32_t cr0_paging_set;
	int i;

	NUM_PE = mem_size() / PAGE_SIZE;
	NUM_PT = NUM_PE / 1024;
	//NUM_PT == 32

	//페이지 디렉토리 구성
	page_dir[0] = (uint32_t)page_tbl | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;

	NUM_PE = RKERNEL_HEAP_START / PAGE_SIZE;
	//NUM_PE == 8192
	//페이지 테이블 구성
	for ( i = 0; i < NUM_PE; i++ ) {
		page_tbl[i] = (PAGE_SIZE * i)
			| PAGE_FLAG_RW
			| PAGE_FLAG_PRESENT;
		//writable & present
	}


	//CR0레지스터 설정
	cr0_paging_set = read_cr0() | CR0_FLAG_PG;		// PG bit set

	//컨트롤 레지스터 저장
	write_cr3( (unsigned)page_dir );		// cr3 레지스터에 PDE 시작주소 저장
	write_cr0( cr0_paging_set );          // PG bit를 설정한 값을 cr0 레지스터에 저장

	reg_handler(14, pf_handler);
}

void memcpy_hard(void* from, void* to, uint32_t len)
{
	write_cr0( read_cr0() & ~CR0_FLAG_PG);
	memcpy(from, to, len);
	write_cr0( read_cr0() | CR0_FLAG_PG);
}

uint32_t* get_cur_pd()
{
	return (uint32_t*)read_cr3();
}

uint32_t pde_idx_addr(uint32_t* addr)
{
	uint32_t ret = ((uint32_t)addr & PAGE_MASK_PDE) >> PAGE_OFFSET_PDE;
	return ret;
}

uint32_t pte_idx_addr(uint32_t* addr)
{
	uint32_t ret = ((uint32_t)addr & PAGE_MASK_PTE) >> PAGE_OFFSET_PTE;
	return ret;
}
//page directory에서 index 위치의 page table 얻기
uint32_t* pt_pde(uint32_t pde)
{
	uint32_t * ret = (uint32_t*)(pde & PAGE_MASK_BASE);
	return ret;
}
//address에서 page table 얻기
uint32_t* pt_addr(uint32_t* addr)
{
	uint32_t idx = pde_idx_addr(addr);
	uint32_t* pd = get_cur_pd();
	return pt_pde(pd[idx]);
}
//address에서 page 주소 얻기
uint32_t* pg_addr(uint32_t* addr)
{
	uint32_t *pt = pt_addr(addr);
	uint32_t idx = pte_idx_addr(addr);
	uint32_t *ret = (uint32_t*)(pt[idx] & PAGE_MASK_BASE);
	return ret;
}

/*
   page table 복사
 */
/*
   void  pt_copy(uint32_t *pd, uint32_t *dest_pd, uint32_t idx)
   {
   uint32_t *pt = pt_pde(pd[idx]);
   uint32_t *new_pt;
   uint32_t i;

//	pt = RH_TO_VH(pt);
new_pt = palloc_get_one_page(kernel_area);

for(i = 0; i<1024; i++)
{
if(pt[i] & PAGE_FLAG_PRESENT)
{
//          new_pt = VH_TO_RH(new_pt);
dest_pd[idx] = (uint32_t)new_pt | (pd[idx] & ~PAGE_MASK_BASE & ~    PAGE_FLAG_ACCESS);
//new_pt = RH_TO_VH(new_pt);
new_pt[i] = pt[i];
}
}
}
 */
void pt_copy(uint32_t *pd, uint32_t *dest_pd, uint32_t idx, uint32_t *start, uint32_t *end, bool share)
{
	uint32_t *pt = pt_pde(pd[idx]);
	uint32_t *new_pt;
	uint32_t i, s, e;

	s = pte_idx_addr(start);
	e = pte_idx_addr(end);

	if (e == 0) { 
		e = PAGE_SIZE / sizeof(uint32_t);
	}

	//if (share == false) {
	new_pt = palloc_get_one_page(kernel_area);
	dest_pd[idx] = (uint32_t)new_pt | (pd[idx] & ~PAGE_MASK_BASE & ~PAGE_FLAG_ACCESS);
	/*
	   }
	   else {
	   dest_pd[idx] = pd[idx];
	   return;
	   }
	 */

	for (i = 0; i < e - s; i++) {
		if (pt[i] & PAGE_FLAG_PRESENT) {
			if (share == false) {
				new_pt[i] = scale_up(pt[i], PAGE_SIZE  * PAGE_TBL_SIZE / sizeof(uint32_t))
					| PAGE_FLAG_RW
					| PAGE_FLAG_PRESENT;
				pagememcpy((void *)pt[i], (void *)new_pt[i], PAGE_SIZE);
			}
			else {
				new_pt[i] = pt[i];
			}
		}
	}

}

/* 
   page directory 복사. 
   커널 영역 복사나 fork에서 사용
 */
/*
   void pd_copy(uint32_t* from, uint32_t* to)
   {
   uint32_t i;

   for(i = 0; i < 1024; i++)
   {
   if(from[i] & PAGE_FLAG_PRESENT)
   pt_copy(from, to, i);
   }
   }
 */

void pd_copy(uint32_t *pd, uint32_t *dest_pd, uint32_t idx, uint32_t *start, uint32_t *end, bool share)
{
	uint32_t *tmp = end;
	uint32_t s, e;

	s = pde_idx_addr(start);
	e = pde_idx_addr(end);

	if (pte_idx_addr(end) != 0) {
		e++;
	}

	end = (uint32_t)0;

	for (idx = s; idx < e; idx++) {
		if (idx == e - 1) {
			end = tmp;
		}

		if (pd[idx] & PAGE_FLAG_PRESENT) {
			pt_copy(pd, dest_pd, idx, start, end, share);
		}

		if (idx == s) {
			start = (uint32_t *)0;
		}
	}

}

uint32_t* pd_create (pid_t pid)
{
	uint32_t *pd = palloc_get_one_page(kernel_area);

	//pd_copy((uint32_t*)read_cr3(), pd);
	//pd = VH_TO_RH(pd);
	pd_copy(get_cur_pd(), pd, NULL, (uint32_t *)KERNEL_ADDR, (uint32_t *)USER_POOL_START, false);

	return pd;
}

void pf_handler(struct intr_frame *iframe)
{
	void *fault_addr;

	asm ("movl %%cr2, %0" : "=r" (fault_addr));
#ifdef SCREEN_SCROLL
	//refreshScreen();
#endif

	uint32_t pdi, pti;
	uint32_t *pta;
	uint32_t *pda = get_cur_pd(); 

	pdi = pde_idx_addr((uint32_t *)fault_addr);
	pti = pte_idx_addr((uint32_t *)fault_addr);

	if(pda == PID0_PAGE_DIR){
		write_cr0( read_cr0() & ~CR0_FLAG_PG);
		pta = pt_pde(pda[pdi]);
		write_cr0( read_cr0() | CR0_FLAG_PG);
	}
	else{
		pta = pt_pde(pda[pdi]);
	}
	//printk("!!%x %x %x %x\n", pta, pti, pda, pdi);

	if(pta == NULL){
		write_cr0( read_cr0() & ~CR0_FLAG_PG);

		pta = palloc_get_one_page(kernel_area);
		memset(pta,0,PAGE_SIZE);

		pda[pdi] = (uint32_t)pta | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;

		pta[pti] = (uint32_t)fault_addr | PAGE_FLAG_RW  | PAGE_FLAG_PRESENT;

		pdi = pde_idx_addr(pta);
		pti = pte_idx_addr(pta);

		uint32_t *tmp_pta = pt_pde(pda[pdi]);
		tmp_pta[pti] = (uint32_t)pta | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;

		write_cr0( read_cr0() | CR0_FLAG_PG);
	}
	else{
		//pta = RH_TO_VH(pta);
		//fault_addr = VH_TO_RH(fault_addr);
		pta[pti] = (uint32_t)fault_addr | PAGE_FLAG_RW  | PAGE_FLAG_PRESENT;
	}

}
