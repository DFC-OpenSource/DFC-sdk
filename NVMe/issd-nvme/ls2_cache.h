
#ifndef __LS2_CACHE_H
#define __LS2_CACHE_H

#include "nvme_bio.h"

#define LBA_FREE		0
#define LBA_IN_CACHE		1
#define LBA_IN_CACHE_SEND_NAND	2
#define LBA_IN_CACHE_NAND	4
#define LBA_IN_NAND		8
#define LBA_TRIMMED		16
#define LBA_IN_UNALIGNED_CACHE	32
//#define LBA_REPEATED		32

#define LS2_DDR_CACHE_SIZE	0x80000000
#define DDR_CACHE_MEM_ADDR	0x8300000000

#define NOT_USED	1
#define USED		2
#define FLUSHED		4

enum QDMA_STATUS {
	FREE   = 0,
	COPIED = 1,
};

#define INVALID	-1

#define LBA_AVAIL_IN_CACHE (LBA_IN_CACHE | LBA_IN_CACHE_NAND)
#define CACHE_AVAIL_BITS (NOT_USED | FLUSHED)

#define IS_CACHED(stat)	(stat & LBA_AVAIL_IN_CACHE)

#define IS_CACHE_AVAIL(status) (status & CACHE_AVAIL_BITS)

typedef struct LBA_Track {
	uint32_t cache_idx:28;
	uint8_t status:6;
} LBA_Track;

typedef struct Cache_DDR {
	uint64_t lba:61;
	uint8_t status:3;
	nvme_bio *bio;
	uint64_t phy_addr;
} Cache_DDR;

typedef struct bio_list {
	nvme_bio *bio;
	struct bio_list *next;
} bio_list;

typedef struct LS2_Cache {
	LBA_Track *lba_track_table;
	Cache_DDR *ddr_table;
	bio_list *head;
	bio_list *tail;
	pthread_mutex_t lba_track_table_mutex;
	pthread_mutex_t cache_ddr_mutex;
	pthread_mutex_t qDMA_mutex;
	pthread_mutex_t cached_bio_mutex;
	uint64_t ls2_ddr_addr;
	uint32_t nr_pages_in_cache;
	uint32_t cache_head_idx;
	uint32_t cache_tail_idx;
	uint32_t qDMA_head_idx;
	uint32_t qDMA_tail_idx;
	uint32_t nr_cached_bio_to_nand;
	int      ls2_mem_fd;
	uint8_t *qDMA_status;
} LS2_Cache;

void nvme_cache_cb(nvme_bio *bio, int ret);
void flash_to_nand(LS2_Cache *cache, nvme_bio *bio, int mq_txid);
void add_to_list(LS2_Cache *cache, nvme_bio *bio);
nvme_bio *remove_from_list(LS2_Cache *cache);
void update_lba_table(LS2_Cache *cache, nvme_bio *bio, uint64_t idx);
void lba_flush_to_nand();

#endif
