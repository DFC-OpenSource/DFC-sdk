
#ifndef __QDMA_H
#define __QDMA_H

#include <stdio.h>

#define QDMA_AVAIL	0x1
#define QDMA_FILLED	0x2
#define QDMA_POSTED	0x4
#define IO_TABLE_SIZE	512

#ifndef CIRCULAR_INCR
#define CIRCULAR_INCR(idx, limit) (idx = (idx + 1) & (limit - 1))
#endif

typedef struct qdma_bio {
	void    *req;
	uint64_t prp[128];
	uint64_t ddr_addr[128];
	uint32_t nlb;
	uint8_t  req_type;
	uint8_t  ualign;
	uint64_t  size;
	uint64_t  *lba;
} qdma_bio;

struct qdma_io
{
    volatile struct qdma_desc *q_desc[128];
    volatile void *req;
    volatile struct timeval   io_post_time;
    volatile uint64_t cache_idx;
    volatile uint32_t count;
    volatile uint16_t status;
    volatile uint16_t chan_no;
    volatile qdma_bio *q_bio;
    pthread_mutex_t qdesc_mutex;
};

typedef struct QdmaCtrl
{
	volatile struct qdma_io des_io_table[IO_TABLE_SIZE];
	struct timeval time1;
	struct timeval time2;
	int volatile producer_index;
	int volatile consumer_index;
	int volatile qdma_io_posted;
	int volatile qdma_io_completed;
	int volatile pi_cnt;
	int volatile ci_cnt;
	int dev_error;
	int volatile size_counters[QDMA_DESC_COUNT];
	pthread_mutex_t qdma_mutex;
	int qdma_mq_txid;
} QdmaCtrl;


void *io_completer_qdma(void *arg);

#ifdef QDMA
//int ramdisk_prp_rw_prologue(NvmeRequest *req, uint64_t idx, uint8_t ualign);
int avail_qdma_desc(uint32_t nlb);
void ramdisk_prp_rw_epilogue();
#endif

#endif
