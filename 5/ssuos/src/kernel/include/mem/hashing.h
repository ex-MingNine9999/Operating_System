#ifndef __HASHING_H__
#define __HASHING_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <type.h>
#include <proc/proc.h>
#include <ssulib.h>

#define SLOT_NUM 4							// The number of slots in a bucket
#define CAPACITY 256						// level hash table capacity

#define SWITCHMODE(MODE) ((MODE + 1) % 2)	// 0 -> 1, 1 -> 0

typedef struct entry{
    uint32_t key;
    uint32_t value;
} entry;

typedef struct level_bucket
{
    uint8_t token[SLOT_NUM];
    entry slot[SLOT_NUM];
} level_bucket;

typedef struct level_hash {
    level_bucket top_buckets[CAPACITY];
    level_bucket bottom_buckets[CAPACITY / 2];
} level_hash;

level_hash hash_table;

void init_hash_table(void);
uint32_t F_IDX(uint32_t addr, uint32_t capacity);	// Get first index to use at table
uint32_t S_IDX(uint32_t addr, uint32_t capacity);	// Get second index to use at table

int find_top_buckets(uint32_t idx, int mode, entry *e);		
int find_bottom_buckets(uint32_t idx, int mode, entry *e);
int search_all_table(void *pages, entry *e, uint32_t *idx, int mode);
int insert_hash_table(void *pages, size_t page_idx);
int remove_hash_table(void *pages, size_t key);
int move_hash_table(void *pages, size_t page_idx, uint32_t *idx);
#endif
