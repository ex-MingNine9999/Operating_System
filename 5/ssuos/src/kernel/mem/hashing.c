#include <device/io.h>
#include <mem/mm.h>
#include <mem/paging.h>
#include <device/console.h>
#include <proc/proc.h>
#include <interrupt.h>
#include <mem/palloc.h>
#include <ssulib.h>
#include <mem/hashing.h>

uint32_t F_IDX(uint32_t addr, uint32_t capacity) {
	return addr % ((capacity / 2) - 1);
}

uint32_t S_IDX(uint32_t addr, uint32_t capacity) {
	return (addr * 7) % ((capacity / 2) - 1) + capacity / 2;
}

void init_hash_table(void)
{
	// TODO : OS_P5 assignment
	uint32_t i, j;

	for (i = 0; i < CAPACITY; i++) {
		for (j = 0; j < SLOT_NUM; j++) {
			hash_table.top_buckets[i].token[j] = 0;
			hash_table.top_buckets[i].slot[j].key = 0;
			hash_table.top_buckets[i].slot[j].value = 0;
			if (i % 2 == 0) {
				hash_table.bottom_buckets[i / 2].token[j] = 0;
				hash_table.bottom_buckets[i / 2].slot[j].key = 0;
				hash_table.bottom_buckets[i / 2].slot[j].value = 0;
			}
		}
	}
}

int find_top_buckets(uint32_t idx, int mode, entry *e)
{
	int i, ret = -1;

	for (i = 0; i < SLOT_NUM; i++) {
		if (hash_table.top_buckets[idx].token[i] == mode) {
			if (mode == 1 && hash_table.top_buckets[idx].slot[i].key != e->key) {
				continue;
			}

			hash_table.top_buckets[idx].token[i] = SWITCHMODE(mode);

			if (mode == 0) {
				hash_table.top_buckets[idx].slot[i] = *e;
			}
			else {
				e->value = hash_table.top_buckets[idx].slot[i].value;
			}

			ret = 0;
			break;
		}
	}

	return ret;
}

int find_bottom_buckets(uint32_t idx, int mode, entry *e)
{
	int i, ret = -1;

	for (i = 0; i < SLOT_NUM; i++) {
		if (hash_table.bottom_buckets[idx / 2].token[i] == mode) {
			if (mode == 1 && hash_table.top_buckets[idx].slot[i].key != e->key) {
				continue;
			}

			hash_table.bottom_buckets[idx / 2].token[i] = SWITCHMODE(mode);

			if (mode == 0) {
				hash_table.bottom_buckets[idx / 2].slot[i] = *e;
			}
			else {
				e->value = hash_table.bottom_buckets[idx / 2].slot[i].value;
			}

			ret = 1;
			break;
		}
	}

	return ret;
}

int search_all_table(void *pages, entry *e, uint32_t *idx, int mode)
{
	int check = -1;

	*idx = F_IDX((uint32_t)pages, CAPACITY);
	check = find_top_buckets(*idx, mode, e);
	if (check != -1) {
		return check;
	}

	*idx = S_IDX((uint32_t)pages, CAPACITY);
	check = find_top_buckets(*idx, mode, e);
	if (check != -1) {
		return check;
	}

	*idx = F_IDX((uint32_t)pages, CAPACITY);
	check = find_bottom_buckets(*idx, mode, e);
	if (check != -1) {
		return check;
	}

	*idx = S_IDX((uint32_t)pages, CAPACITY);
	check = find_bottom_buckets(*idx, mode, e);
	if (check != -1) {
		return check;
	}

	return check;
}

int insert_hash_table(void *pages, size_t page_idx)
{
	entry e;
	uint32_t i, idx;
	const char TB[2][7] = {"top", "bottom"};
	int check = -1;

	e.key = page_idx;
	e.value = VH_TO_RH(pages);

	check = search_all_table(pages, &e, &idx, 0);

	if (check < 0) {
		check = move_hash_table(pages, page_idx, &idx);
	}

	if (check >= 0) {
		if (check == 1) {
			idx /= 2;
		}

		printk("hash value inserted in %s level : idx : %d, key : %d, value : %x\n", TB[check], idx, e.key, e.value);
		return 0;
	}

	printk("hash table need resize\n");

	return -1;
}

int remove_hash_table(void *pages, size_t key)
{
	uint32_t i, idx;
	int check = -1;
	entry e;

	e.key = key;

	check = search_all_table(pages, &e, &idx, 1);

	if (check >= 0) {
		if (check == 1) {
			idx /= 2;
		}

		printk("hash value deleted : idx : %d, key : %d, value : %x\n", idx, e.key, e.value);
	}

	return 0;
}

int move_hash_table(void *pages, size_t page_idx, uint32_t *idx)
{
	uint32_t i, tmpIdx;
	int ret = -1, check = -1;
	entry e;
	void *tmpPage;

	*idx = F_IDX((uint32_t)pages, CAPACITY);
	for (i = 0; i < SLOT_NUM; i++) {
		if (hash_table.top_buckets[*idx].token[i] == 1) {
			e = hash_table.top_buckets[*idx].slot[i];
			
			tmpPage = RH_TO_VH(e.value);
			if ((check = search_all_table(tmpPage, &e, &tmpIdx, 0)) != -1) {
				printk("hash value moved top : idx : %d new idx : %d\n", *idx, tmpIdx);
				hash_table.top_buckets[*idx].token[i] = 0;
				e.key = page_idx;
				e.value = VH_TO_RH(pages);
				return search_all_table(pages, &e, idx, 0);
			}
		}
	}

	*idx = S_IDX((uint32_t)pages, CAPACITY);
	for (i = 0; i < SLOT_NUM; i++) {
		if (hash_table.top_buckets[*idx].token[i] == 1) {
			e = hash_table.top_buckets[*idx].slot[i];
			
			tmpPage = RH_TO_VH(e.value);
			if ((check = search_all_table(tmpPage, &e, &tmpIdx, 0)) != -1) {
				hash_table.top_buckets[*idx].token[i] = 0;
				printk("hash value moved top : idx : %d new idx : %d\n", *idx, tmpIdx);
				e.key = page_idx;
				e.value = VH_TO_RH(pages);
				return search_all_table(pages, &e, idx, 0);
			}
		}
	}

	*idx = F_IDX((uint32_t)pages, CAPACITY);
	for (i = 0; i < SLOT_NUM; i++) {
		if (hash_table.bottom_buckets[*idx / 2].token[i] == 1) {
			e = hash_table.bottom_buckets[*idx / 2].slot[i];
			
			tmpPage = RH_TO_VH(e.value);
			if ((check = search_all_table(tmpPage, &e, &tmpIdx, 0)) != -1) {
				printk("hash value moved bottom : idx : %d new idx : %d\n", *idx / 2, tmpIdx / 2);
				hash_table.bottom_buckets[*idx / 2].token[i] = 0;
				e.key = page_idx;
				e.value = VH_TO_RH(pages);
				return search_all_table(pages, &e, idx, 0);
			}
		}
	}

	*idx = S_IDX((uint32_t)pages, CAPACITY);
	for (i = 0; i < SLOT_NUM; i++) {
		if (hash_table.bottom_buckets[*idx / 2].token[i] == 1) {
			e = hash_table.bottom_buckets[*idx / 2].slot[i];
			
			tmpPage = RH_TO_VH(e.value);
			if ((check = search_all_table(tmpPage, &e, &tmpIdx, 0)) != -1) {
				printk("hash value moved bottom : idx : %d new idx : %d\n", *idx / 2, tmpIdx / 2);
				hash_table.bottom_buckets[*idx / 2].token[i] = 0;
				e.key = page_idx;
				e.value = VH_TO_RH(pages);
				return search_all_table(pages, &e, idx, 0);
			}
		}
	}

	return check;
}
