#ifndef U_CACHE
#define U_CACHE

#include "common.h"
#include "nvme_bio.h"

#define POWER_FACTER(x,y) \
        do { \
                if (((x) = ((x) >> 1))) (y)++; \
        } while ((x) > 1) ;

#define UNALIGNED_CACHE_PAGES   512
#define SECTORS_PER_PAGE        8
#define KPAGES_PER_FPAGE        8
#define UNALIGNED_CACHE_PHY_ADDR        0x8380000000

#define UNALIGNED_FREED   1
#define UNALIGNED_USED    2
#define UNDER_PROCESS	  4
#define	UNALIGNED_TRIMMED 8

#define	MEM_FREED 1
#define	MEM_USED 2

typedef struct ftl_bio {
	uint64_t    lba[8];
	uint8_t     is_seq;
	uint16_t    nlb;
	uint16_t    req_type;
	void        *req_info;
	uint64_t    prp[128];
}ftl_bio;

typedef struct Unaligned_Cache_DDR {
	uint64_t	lba;
	uint64_t        phy_addr;
	volatile uint8_t 	status;
	uint64_t     *virt_addr;
	pthread_mutex_t page_lock;
} unaligned_cache_ddr;

typedef struct ualn_cache{
	unaligned_cache_ddr 	*cache_table;
	volatile uint16_t 	head_idx;
	volatile uint16_t 	tail_idx;
	volatile uint16_t 	f_idx;
	uint8_t 	ls2_mem_fd;
	uint64_t 	ls2_virt_addr;
	pthread_mutex_t bdry_lock;
	pthread_mutex_t f_idx_lock;
	pthread_mutex_t ls2_mem_lock;
	volatile uint8_t 	page_boundary;
	volatile uint16_t mem_head;
	volatile uint16_t mem_tail;
	volatile uint8_t ls2_memory_status[UNALIGNED_CACHE_PAGES];
	uint64_t ls2_phy_addr[UNALIGNED_CACHE_PAGES];
} nvme_cache;


void enqueue_bio_to_mq(void *bio, int mq_txid);
int flush_to_nand();
void *unalign_bio_process(void *arg);
void remove_the_hole(nvme_cache *cache);

#endif
