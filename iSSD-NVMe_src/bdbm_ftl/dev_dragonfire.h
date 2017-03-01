#ifndef __DF_DEV_H
#define __DF_DEV_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "bdbm_drv.h"
#include "list.h"
#include "debug.h"

/* define the following as 0 if integrating with ftl */
//#define THIS_IS_SIMULATION 1

#define KB(n)  (1024*n)
#define MB(n)  (1024*KB(n))

#define DESC_AWAITED     1
#define DESC_NOT_AWAITED 0

#ifndef KPAGE_SIZE
#define KPAGE_SIZE 4096
#endif

/* Magic key:
 * 155D - iSSD
 * DF   - DragonFire
 * FF   - Dual purpose
 *        1. 1st byte of 1st page spare of block indicates block is good/bad
 *        2. 1st byte of magic key to complete 8 byte key
 */
#define MAGIC_KEY        0x155DDFFF

#define JUMP_ON_ERR(errst, ret, label, fmt, args...) \
	do {                                               \
		if (0 != (ret = errst)) {           \
			if (*fmt) printf (fmt, ##args);            \
			goto label;                                \
		}                                              \
	} while (0)
#if 0
#if THIS_IS_SIMULATION
#define NR_CHANNELS           4
#define NR_CHIPS_PER_CHANNEL  1
#define NR_BLOCKS_PER_CHIP    128
#define NR_PAGES_PER_BLOCK    256
#define PAGE_MAIN_SIZE        512
#define PAGE_SPARE_SIZE       64
#else
#define NR_CHANNELS           16
#define NR_CHIPS_PER_CHANNEL  1
#define NR_BLOCKS_PER_CHIP    2096
#define NR_PAGES_PER_BLOCK    512
#define PAGE_MAIN_SIZE        16384
#define PAGE_SPARE_SIZE       1872
#endif
#endif
enum {
	BDBM_ST_DIMMS_FRESH    = 1,
	BDBM_ST_DIMMS_UNKNOWN  = 2,
	BDBM_ST_DIMMS_BADTAGS  = 3,
	BDBM_ST_DIMMS_DIFFTAGS = 4,
	BDBM_ST_DIMMS_SWAPPED  = 5,
	BDBM_ST_RESCANNED      = 6,
	BDBM_ST_LOAD_SUCCESS   = 7,
	BDBM_ST_LOAD_FAILED    = 8,
	BDBM_ST_STORE_SUCCESS  = 9,
	BDBM_ST_STORE_FAILED   = 10,
	BDBM_ST_DIMMS_NEW      = 11,
	BDBM_ST_DIMMS_MISSING  = 12,
	BDBM_ST_EXIT_GOOD      = 0xfe,
	BDBM_ST_EXIT_BAD       = 0xff,
};

enum BDBM_PFTL_PAGE_STATUS {
	PFTL_PAGE_NOT_ALLOCATED = 0,
	PFTL_PAGE_VALID,
	PFTL_PAGE_INVALID,
	PFTL_PAGE_INVALID_ADDR = -1,
};

typedef struct local_mem {
	uint8_t  *phy;
	uint8_t  *virt;
} local_mem_t;

/* a physical address */
typedef struct bdbm_nandaddr {
	bdbm_phyaddr_t row;
	uint32_t       col; /*offset from a page or to spare*/
} bdbm_nandaddr_t;

typedef struct bdbm_page_mapping_entry {
	uint8_t status; /* BDBM_PFTL_PAGE_STATUS */
	bdbm_phyaddr_t phyaddr; /* physical location */
	uint8_t sp_off;
} bdbm_page_mapping_entry_t;

typedef bdbm_page_mapping_entry_t bdbm_pme_t;

typedef struct bdbm_npme {
	uint8_t  channel;
	uint8_t  chip;
	uint16_t block;
	uint16_t page;
	uint8_t sp_off;
	uint8_t  status;
} bdbm_npme_t;

typedef struct bdbm_device_params {
	uint64_t nr_channels; //FPGA
	uint64_t nr_chips_per_channel; //FPGA
	uint64_t nr_blocks_per_chip; // UNFC
	uint64_t nr_pages_per_block; //UNFC
	uint64_t page_main_size; //UNFC
	uint64_t page_oob_size; //UNFC
	uint64_t subpage_size; //UNFC
	uint32_t device_type;//UNFC
	uint8_t  timing_mode;
	uint64_t device_capacity_in_byte; //calculated
	uint64_t page_prog_time_us;
	uint64_t page_read_time_us;
	uint64_t block_erase_time_us;

	uint64_t nr_blocks_per_channel; // calculated
	uint64_t nr_blocks_per_ssd;
	uint64_t nr_chips_per_ssd;
	uint64_t nr_pages_per_ssd;
	uint64_t nr_subpages_per_block;
	uint64_t nr_subpages_per_page;
	uint64_t nr_subpages_per_ssd; /* subpage size must be4 KB */
} bdbm_device_params_t;

typedef struct bdbm_dimm_status {
	uint8_t  nr_dimms;
	uint32_t dimm_tags[2];
} bdbm_dimm_status_t;

typedef uint8_t babm_abm_subpage_t; /* BDBM_ABM_PAGE_STATUS */

enum BDBM_ABM_SUBPAGE_STATUS {
	BABM_ABM_SUBPAGE_NOT_INVALID = 0,
	BDBM_ABM_SUBPAGE_INVALID,
};

enum BDBM_ABM_BLK_STATUS {
	BDBM_ABM_BLK_FREE = 0,
	BDBM_ABM_BLK_FREE_PREPARE,
	BDBM_ABM_BLK_CLEAN,
	BDBM_ABM_BLK_DIRTY,

	BDBM_ABM_BLK_MGMT,
	BDBM_ABM_BLK_BAD,
};

typedef struct bdbm_abm_block {
	uint8_t status; /* ABM_BLK_STATUS */
	uint8_t channel;
	uint8_t chip;
	uint16_t block;
	uint32_t erase_count;
	uint16_t nr_invalid_subpages;
	babm_abm_subpage_t* pst;   /* a page status table; used when the FTL requires */

	struct list_head list;  /* for list */
} bdbm_abm_block_t;

typedef struct bdbm_nblock_st {
	uint8_t channel;
	uint8_t chip;
	uint16_t block;
	uint32_t erase_count;
	uint16_t nr_invalid_subpages;
	uint8_t status; /* ABM_BLK_STATUS */
} bdbm_nblock_st_t;

typedef struct bdbm_abm_info {
	struct bdbm_device_params* np;
	bdbm_abm_block_t*   blocks;
	struct list_head**  list_head_free;
	struct list_head**  list_head_clean;
	struct list_head**  list_head_dirty;
	struct list_head**  list_head_mgmt;
	struct list_head**  list_head_bad;

	/*struct list_head**  list_head_pmt_blocks;*/

	/* # of blocks according to their types */
	uint64_t nr_total_blks;
	uint64_t nr_free_blks;
	uint64_t nr_free_blks_prepared;
	uint64_t nr_clean_blks;
	uint64_t nr_dirty_blks;
	uint64_t nr_mgmt_blks;
	uint64_t nr_bad_blks;

	uint16_t *punits_iobb_pool; /*Block boundary for each chip*/
} bdbm_abm_info_t;

typedef struct bdbm_mgmt_dptr {
	bdbm_nandaddr_t addr;
	uint32_t       nEntries;
} bdbm_mgmt_dptr_t;

#define MGMTB_MGMT_PTR_PAGE 1

/* various status holder offsets in magic block page-1 */
typedef enum mgmt_list_type {
	MGMTB_PMTB_CURR = 0,
	MGMTB_PMTB_POOL = 1,
	MGMTB_BMT       = 2,
	MGMTB_IOBB_POOL	= 3,
	MGMTB_MAX       = 4,
} mgmt_list_type_t;

#define CIRC_INCR(idx, limit) (idx = (idx + 1) % limit)
#define CIRC_INCR_PTR(ptr, head, limit) \
	(ptr = head + (ptr - head + 1) % limit)

typedef struct bdbm_dev_private {
	bdbm_pme_t            *pmt;
	bdbm_abm_info_t       *bai;

#if THIS_IS_SIMULATION
	uint8_t               *ssdmem;
	uint64_t              ssdsz;
#endif

	int                   dimm_status;
	bdbm_dimm_status_t    dimms;

	bdbm_device_params_t  nand;

	uint64_t              nr_mgmt_blocks_per_ssd;
	bdbm_nblock_st_t      **mgmt_blocks;
	bdbm_nblock_st_t      *bmi[MGMTB_MAX];
	bdbm_mgmt_dptr_t      mgmt_blkptr[MGMTB_MAX];
} bdbm_dev_private_t;

static inline
uint64_t __get_channel_ofs (bdbm_device_params_t* np, uint64_t blk_idx) {
	return (blk_idx / np->nr_blocks_per_channel);
}

static inline
uint64_t __get_chip_ofs (bdbm_device_params_t* np, uint64_t blk_idx) {
	return ((blk_idx % np->nr_blocks_per_channel) / np->nr_blocks_per_chip);
}

static inline
uint64_t __get_block_ofs (bdbm_device_params_t* np, uint64_t blk_idx) {
	return (blk_idx % np->nr_blocks_per_chip);
}

static inline
uint64_t __get_block_idx (bdbm_device_params_t* np, uint64_t channel, uint64_t chip, uint64_t block) {
	return channel * np->nr_blocks_per_channel +
		chip * np->nr_blocks_per_chip +
		block;
}

int bdbm_get_dimm_status (bdbm_dev_private_t *pdp);
int setup_new_ssd_context (bdbm_dev_private_t *pdp);
int bdbm_ftl_load_from_flash (bdbm_dev_private_t *pdp);
int bdbm_ftl_store_to_flash (bdbm_dev_private_t *pdp);
void bdbm_scan_bb_completion_cb (void *req_ptr, uint8_t *Buff, uint8_t ret);
#endif
